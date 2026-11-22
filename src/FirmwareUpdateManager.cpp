#include "FirmwareUpdateManager.h"

#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>

#include <memory>
#include <new>

#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "ServerManager.h"
#include "globals.h"

#ifdef DEBUG
#define FIRMWARE_KIND "debug"
#else
#define FIRMWARE_KIND "release"
#endif

// Every firmware that can be installed over the air carries this text, so an upload without it is not
// one: a random file, or an NSClock version from before updates, which expects another flash layout.
// It also names the version and kind, which is how the clock knows what its other slot holds.
#define MARKER_PREFIX "NSCLOCK-OTA-1:"
const char firmwareMarker[] __attribute__((used)) = MARKER_PREFIX VERSION ":" FIRMWARE_KIND;
// The prefix as a literal only inside sizeof, so the marker is its only copy in the firmware.
static constexpr size_t MARKER_PREFIX_LENGTH = sizeof(MARKER_PREFIX) - 1;

// The page asks to start, then has this long to begin the upload.
static constexpr unsigned long READY_TIMEOUT_MS = 60000;
// An upload that sends nothing for this long has stalled, for example because the page was closed.
static constexpr unsigned long STALL_TIMEOUT_MS = 15000;
// A newly installed firmware that runs this long, connected or in setup mode, is kept.
static constexpr unsigned long CONFIRM_AFTER_MS = 60000;
static constexpr unsigned long SUCCESS_SHOWN_MS = 2000;
static constexpr unsigned long FAILURE_SHOWN_MS = 3000;
static constexpr size_t SCAN_CHUNK = 4096;

// The core marks a new firmware as working before setup() runs, so one that crashes in setup() would
// restart forever. Returning true leaves that to confirmRunningFirmware(): until then, a restart
// makes the bootloader return to the previous firmware.
extern "C" bool verifyRollbackLater() { return true; }

FirmwareUpdateManager_& FirmwareUpdateManager_::getInstance() {
    static FirmwareUpdateManager_ instance;
    return instance;
}

FirmwareUpdateManager_& FirmwareUpdateManager = FirmwareUpdateManager_::getInstance();

// Advances a search for the marker's prefix by one byte; true when the prefix is complete. The
// prefix's first character appears nowhere else in it, so a mismatch can only restart the match.
static bool matchMarkerByte(size_t& matched, uint8_t byte) {
    if (byte == static_cast<uint8_t>(firmwareMarker[matched])) {
        matched++;
    } else {
        matched = byte == static_cast<uint8_t>(firmwareMarker[0]) ? 1 : 0;
    }
    if (matched == MARKER_PREFIX_LENGTH) {
        matched = 0;
        return true;
    }
    return false;
}

// Runs in ESP.restart(), not after a crash. A restart the firmware chose, such as after a data
// source's login or a factory reset, does not mean the firmware fails.
static void confirmBeforeRestart() { FirmwareUpdateManager.confirmRunningFirmware(); }

void FirmwareUpdateManager_::setup() {
    esp_register_shutdown_handler(confirmBeforeRestart);
    findPreviousFirmware();
}

void FirmwareUpdateManager_::findPreviousFirmware() {
    const esp_partition_t* other = esp_ota_get_next_update_partition(NULL);
    if (other == nullptr) {
        return;
    }
    // The whole image, with its hash: the other slot is empty after an install that erased the flash,
    // and an update that failed leaves a partial or corrupt image there.
    const esp_partition_pos_t position = {other->address, other->size};
    esp_image_metadata_t image;
    if (esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &position, &image) != ESP_OK) {
        DEBUG_PRINTLN("The other firmware slot holds no complete firmware");
        return;
    }
    // A version that failed to start is not offered again, but the page warns before installing it.
    esp_ota_img_states_t otaState;
    const bool failed = esp_ota_get_state_partition(other, &otaState) == ESP_OK &&
                        (otaState == ESP_OTA_IMG_INVALID || otaState == ESP_OTA_IMG_ABORTED);

    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[SCAN_CHUNK]);
    if (!buffer) {
        return;
    }
    size_t matched = 0;
    for (size_t offset = 0; offset + SCAN_CHUNK <= other->size; offset += SCAN_CHUNK) {
        if (esp_partition_read(other, offset, buffer.get(), SCAN_CHUNK) != ESP_OK) {
            return;
        }
        for (size_t i = 0; i < SCAN_CHUNK; i++) {
            if (!matchMarkerByte(matched, buffer[i])) {
                continue;
            }
            // "<version>:<kind>" follows the prefix.
            char rest[48] = {0};
            if (esp_partition_read(other, offset + i + 1, rest, sizeof(rest) - 1) != ESP_OK) {
                return;
            }
            String text(rest);
            int colon = text.lastIndexOf(':');
            String kind = colon > 0 ? text.substring(colon + 1) : "";
            if (kind != "debug" && kind != "release") {
                continue;
            }
            previousVersion = text.substring(0, colon);
            previousDebug = kind == "debug";
            previousAvailable = !failed;
            previousFailed = failed;
            DEBUG_PRINTF(
                "The other firmware slot holds %s (%s)%s", previousVersion.c_str(), kind.c_str(),
                failed ? ", which failed to start" : "");
            return;
        }
    }
}

const char* FirmwareUpdateManager_::stateName(State state) {
    switch (state) {
        case State::IDLE:
            return "idle";
        case State::STOPPING:
            return "stopping";
        case State::READY:
            return "ready";
        case State::RECEIVING:
            return "receiving";
        case State::SUCCEEDED:
            return "succeeded";
        case State::FAILED:
            return "failed";
    }
    return "unknown";
}

void FirmwareUpdateManager_::addRoutes(AsyncWebServer* ws) {
    // Stops the data source and shows UPDATE; the page polls /api/update/status until "ready".
    ws->on(
        AsyncURIMatcher::exact("/api/update/start"), HTTP_POST, [this](AsyncWebServerRequest* request) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            // Not after going back either: the upload would overwrite the slot the restart boots.
            State expected = State::IDLE;
            if (goingBack || !state.compare_exchange_strong(expected, State::STOPPING)) {
                request->send(
                    409, "application/json",
                    String("{\"status\": \"error\", \"state\": \"") + stateName(expected) + "\"}");
                return;
            }
            // A firmware that receives an update works. If this one is new and still unconfirmed, a
            // failed update must not make the bootloader return to the slot the update overwrites.
            confirmRunningFirmware();
            request->send(202, "application/json", "{\"status\": \"ok\", \"state\": \"stopping\"}");
        });

    ws->on(
        AsyncURIMatcher::exact("/api/update/status"), HTTP_GET, [this](AsyncWebServerRequest* request) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            State current = state;
            String body = String("{\"state\": \"") + stateName(current) + "\"";
            if (current == State::FAILED) {
                body += ", \"error\": \"" + failureText() + "\"";
            }
            body += "}";
            request->send(200, "application/json", body);
        });

    // The firmware file, as a multipart upload, with its size in X-Update-Size.
    ws->on(
        AsyncURIMatcher::exact("/api/update"), HTTP_POST,
        [this](AsyncWebServerRequest* request) { finishUploadRequest(request); },
        [this](
            AsyncWebServerRequest* request, const String&, size_t index, uint8_t* data, size_t len,
            bool final) { handleUpload(request, index, data, len, final); });

    ws->on(
        AsyncURIMatcher::exact("/api/update/previous"), HTTP_GET,
        [this](AsyncWebServerRequest* request) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            String body = String("{\"available\": ") + (previousAvailable ? "true" : "false");
            if (previousAvailable || previousFailed) {
                body += String(", \"failed\": ") + (previousFailed ? "true" : "false");
                body += ", \"version\": \"" + previousVersion + "\", \"debug\": ";
                body += previousDebug ? "true" : "false";
            }
            body += "}";
            request->send(200, "application/json", body);
        });

    // Boots the firmware in the other slot. rollBack() checks the whole image before switching to it.
    ws->on(
        AsyncURIMatcher::exact("/api/update/previous"), HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            if (state != State::IDLE || !previousAvailable || goingBack) {
                request->send(
                    409, "application/json",
                    "{\"status\": \"error\", \"error\": \"no previous version\"}");
                return;
            }
            // While this firmware is unconfirmed, switching the boot slot would leave it recorded as one
            // that failed, and the newer version could not be chosen again.
            confirmRunningFirmware();
            if (!Update.rollBack()) {
                DEBUG_PRINTLN("The previous firmware could not be selected");
                request->send(
                    409, "application/json",
                    "{\"status\": \"error\", \"error\": \"the previous version is damaged\"}");
                return;
            }
            DEBUG_PRINTF("Going back to %s", previousVersion.c_str());
            request->send(200, "application/json", "{\"status\": \"ok\"}");
            // From a timer, after the answer has gone out: the main loop can be busy with a data
            // source's request for several seconds.
            goingBack = true;
            const esp_timer_create_args_t timerArgs = {
                .callback = [](void*) { ESP.restart(); }, .arg = nullptr, .name = "go_back"};
            esp_timer_handle_t timer;
            if (esp_timer_create(&timerArgs, &timer) != ESP_OK ||
                esp_timer_start_once(timer, 1000000) != ESP_OK) {
                ESP.restart();
            }
        });
}

void FirmwareUpdateManager_::handleUpload(
    AsyncWebServerRequest* request, size_t index, uint8_t* data, size_t len, bool final) {
    if (index == 0) {
        // Only one upload, from a page that may change settings, once the source has stopped.
        if (!ServerManager.isAuthorized(request)) {
            return;
        }
        // Before RECEIVING is visible, or the main loop could compare it with an old time and declare
        // the upload stalled. A refused second upload sets it too, which only postpones a stall once.
        lastDataMs = millis();
        State expected = State::READY;
        if (!state.compare_exchange_strong(expected, State::RECEIVING)) {
            return;
        }
        uploadRequest = request;
        markerMatched = 0;
        markerFound = false;
        // The file's size, without the multipart framing around it. begin() refuses a file bigger than
        // the slot, and end() one that arrived shorter.
        long size = request->hasHeader("X-Update-Size")
                        ? request->getHeader("X-Update-Size")->value().toInt()
                        : 0;
        if (size <= 0) {
            fail("the upload did not give the firmware's size");
            return;
        }
        DEBUG_PRINTF("Receiving firmware, %ld bytes", size);
        if (!Update.begin(size, U_FLASH)) {
            fail("the update could not start", Update.errorString());
            return;
        }
    }
    if (request != uploadRequest || state != State::RECEIVING) {
        return;
    }
    lastDataMs = millis();
    for (size_t i = 0; i < len && !markerFound; i++) {
        markerFound = matchMarkerByte(markerMatched, data[i]);
    }
    if (Update.write(data, len) != len) {
        fail("writing the firmware failed", Update.errorString());
        return;
    }
    if (!final) {
        return;
    }
    if (!markerFound) {
        Update.abort();
        fail("this is not an NSClock firmware that supports updates");
        return;
    }
    if (Update.remaining() > 0) {
        Update.abort();
        fail("the upload ended before the whole firmware arrived");
        return;
    }
    // end() resets the count.
    size_t received = Update.progress();
    if (!Update.end()) {
        fail("the firmware did not pass its check", Update.errorString());
        return;
    }
    DEBUG_PRINTF("Firmware installed, %u bytes", received);
    state = State::SUCCEEDED;
}

void FirmwareUpdateManager_::finishUploadRequest(AsyncWebServerRequest* request) {
    if (request != uploadRequest) {
        if (!ServerManager.enforceAuthentication(request)) {
            return;
        }
        request->send(
            409, "application/json", "{\"status\": \"error\", \"error\": \"start the update first\"}");
        return;
    }
    if (state == State::SUCCEEDED) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
    } else {
        request->send(
            500, "application/json", "{\"status\": \"error\", \"error\": \"" + failureText() + "\"}");
    }
}

void FirmwareUpdateManager_::fail(const char* reason, const char* detail) {
    // Set before the state, which is what other code reads first.
    failureReason = reason;
    failureDetail = detail;
    state = State::FAILED;
    DEBUG_PRINTF("Firmware update failed: %s", failureText().c_str());
}

void FirmwareUpdateManager_::failIfStill(State current, const char* reason) {
    if (!state.compare_exchange_strong(current, State::FAILED)) {
        return;
    }
    // Set after the state here, so it cannot replace the reason of an upload that failed meanwhile. A
    // status request in between reads "unknown".
    failureReason = reason;
    failureDetail = nullptr;
    DEBUG_PRINTF("Firmware update failed: %s", reason);
}

String FirmwareUpdateManager_::failureText() const {
    const char* reason = failureReason;
    const char* detail = failureDetail;
    String text = reason != nullptr ? reason : "unknown";
    if (detail != nullptr && detail[0] != '\0') {
        text += String(" (") + detail + ")";
    }
    return text;
}

void FirmwareUpdateManager_::showMessage(const char* text) {
    DisplayManager.setPower(true);
    DisplayManager.clearMatrix();
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.printText(0, 6, text, TEXT_ALIGNMENT::CENTER, 1);
    DisplayManager.update();
}

void FirmwareUpdateManager_::confirmRunningFirmware() {
    if (confirmed.exchange(true)) {
        return;
    }
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t otaState;
    if (esp_ota_get_state_partition(running, &otaState) == ESP_OK &&
        otaState == ESP_OTA_IMG_PENDING_VERIFY) {
        if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
            DEBUG_PRINTF("Firmware %s confirmed", VERSION);
        } else {
            DEBUG_PRINTF("Firmware %s could not be confirmed", VERSION);
        }
    }
}

void FirmwareUpdateManager_::takeOverIfStarted() {
    if (!isActive()) {
        return;
    }
    // The caller may be the data source that tick() stops, which is safe only because control never
    // returns to it.
    while (true) {
        ServerManager.tick();
        tick();
        delay(1);
    }
}

void FirmwareUpdateManager_::tick() {
    switch (state.load()) {
        case State::IDLE: {
            if (!confirmed && millis() > CONFIRM_AFTER_MS &&
                (ServerManager.isConnected || ServerManager.isInAPMode)) {
                confirmRunningFirmware();
            }
            return;
        }
        case State::STOPPING:
            // In the main loop, where the source runs, so it is not in the middle of a request.
            bgSourceManager.end();
            showMessage("UPDATE");
            readySinceMs = millis();
            state = State::READY;
            DEBUG_PRINTLN("Ready for the firmware upload");
            return;
        case State::READY:
            if (millis() - readySinceMs > READY_TIMEOUT_MS) {
                // Only if the upload has not started meanwhile.
                failIfStill(State::READY, "no upload arrived");
            }
            return;
        case State::RECEIVING: {
            // Read before millis(): the upload task updates it, and a value newer than the millis()
            // read would make the difference wrap around.
            unsigned long lastData = lastDataMs;
            if (millis() - lastData > STALL_TIMEOUT_MS) {
                // Only if the upload has not ended meanwhile, with its own outcome.
                failIfStill(State::RECEIVING, "the upload stalled");
            }
            return;
        }
        case State::SUCCEEDED:
            showMessage("SUCCESS");
            // Also lets the answer to the upload go out.
            delay(SUCCESS_SHOWN_MS);
            ESP.restart();
            return;
        case State::FAILED:
            // The other slot is not bootable: Update writes an image's first bytes last, and the boot
            // slot changes only when an upload is complete and checked.
            showMessage("FAILED");
            delay(FAILURE_SHOWN_MS);
            ESP.restart();
            return;
    }
}
