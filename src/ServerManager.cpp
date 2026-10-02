#include "ServerManager.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>

#include "BGDisplayManager.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "SettingsManager.h"
#include "globals.h"
#include "time.h"

// The getter for the instantiated singleton instance
ServerManager_& ServerManager_::getInstance() {
    static ServerManager_ instance;
    return instance;
}

// Web authentication constants, cookie set for 10 minutes
static constexpr unsigned long WEB_AUTH_TOKEN_TTL_MS = 10UL * 60UL * 1000UL;
static constexpr int WEB_AUTH_COOKIE_MAX_AGE_SEC = 10 * 60;
static const char* WEB_AUTH_COOKIE_NAME = "auth_token";

static String getCookieValue(const String& cookieHeader, const String& name) {
    int pos = 0;
    while (pos < cookieHeader.length()) {
        int end = cookieHeader.indexOf(';', pos);
        if (end < 0) {
            end = cookieHeader.length();
        }
        String pair = cookieHeader.substring(pos, end);
        pair.trim();
        int eq = pair.indexOf('=');
        if (eq > 0) {
            String key = pair.substring(0, eq);
            key.trim();
            if (key == name) {
                return pair.substring(eq + 1);
            }
        }
        pos = end + 1;
    }
    return "";
}

static String buildAuthCookie(const String& token, int maxAgeSeconds) {
    String cookie = String(WEB_AUTH_COOKIE_NAME) + "=" + token;
    if (maxAgeSeconds >= 0) {
        cookie += "; Max-Age=" + String(maxAgeSeconds);
    }
    cookie += "; Path=/; SameSite=Strict; HttpOnly";
    return cookie;
}

// Initialize the global shared instance
ServerManager_& ServerManager = ServerManager.getInstance();

String ServerManager_::getHostname() {
    if (SettingsManager.settings.hostname != "") {
        return SettingsManager.settings.hostname;
    }

    String hostname = HOSTNAME_PREFIX;

    if (SettingsManager.settings.custom_hostname_enable) {
        hostname = SettingsManager.settings.custom_hostname;
    }

    SettingsManager.settings.hostname = hostname;

    DEBUG_PRINTF("Hostname: %s\n", hostname.c_str());

    return hostname;
}

bool ServerManager_::isWebAuthEnabled() const {
    return SettingsManager.settings.web_auth_enable &&
           SettingsManager.settings.web_auth_password.length() > 0;
}

bool ServerManager_::isRequestAuthenticated(AsyncWebServerRequest* request) const {
    if (!isWebAuthEnabled()) {
        return false;
    }

    if (webAuthToken.length() == 0) {
        return false;
    }

    if (webAuthTokenIssuedMs == 0 || (millis() - webAuthTokenIssuedMs) > WEB_AUTH_TOKEN_TTL_MS) {
        const_cast<ServerManager_*>(this)->webAuthToken = "";
        const_cast<ServerManager_*>(this)->webAuthTokenIssuedMs = 0;
        return false;
    }

    String requestToken = "";
    if (request->hasHeader("Cookie")) {
        requestToken = getCookieValue(request->getHeader("Cookie")->value(), WEB_AUTH_COOKIE_NAME);
    }

    return requestToken.length() > 0 && requestToken == webAuthToken;
}

String ServerManager_::generateAuthToken() {
    uint32_t part1 = esp_random();
    uint32_t part2 = esp_random();
    uint32_t part3 = millis();
    return String(part1, HEX) + String(part2, HEX) + String(part3, HEX);
}

bool ServerManager_::enforceAuthentication(AsyncWebServerRequest* request) {
    if (!isWebAuthEnabled()) {
        return true;
    }

    if (isRequestAuthenticated(request)) {
        return true;
    }

    request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
    return false;
}

IPAddress ServerManager_::setAPmode(String ssid, String psk) {
    auto ipAP = IPAddress();
    auto netmaskAP = IPAddress();
    auto gatewayAP = IPAddress();
    ipAP.fromString(AP_IP);
    netmaskAP.fromString(AP_NETMASK);
    gatewayAP.fromString(AP_GATEWAY);

    apMode = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.softAPConfig(ipAP, gatewayAP, netmaskAP);
    WiFi.softAP(ssid, psk);
    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    return WiFi.softAPIP();
}

void initiateWiFiConnection(String wifi_type, String ssid, String username, String password) {
    if (wifi_type == "wpa_eap") {
        WiFi.begin(ssid, WPA2_AUTH_PEAP, username, username, password);
    } else if (password == "") {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, password);
    }
}

bool tryConnectToWiFi(String wifi_type, String ssid, String username, String password) {
    if (ssid == "") {
        DEBUG_PRINTLN("SSID is empty, cannot connect to WiFi");
        return false;
    }
    int timeout = WIFI_CONNECT_TIMEOUT;

    WiFi.mode(WIFI_STA);

    DEBUG_PRINTF("Connecting to %s (%s)\n", ssid.c_str(), wifi_type.c_str());

    initiateWiFiConnection(wifi_type, ssid, username, password);

    auto startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        // If no connection after a while go in Access Point mode
        if (millis() - startTime > timeout)
            break;
    }

    Serial.println();

    return false;
}

// --- Authenticated network OTA updates -------------------------------------
// Two distinct update types, each with its own endpoint and validation:
//   POST /api/update/firmware   -> U_FLASH  (must be an ESP32 app image, magic 0xE9)
//   POST /api/update/filesystem -> U_SPIFFS (must be exactly the LittleFS partition size)
// Both require web authentication when it is enabled. Auth is checked when the
// upload starts; the final response (and reboot) happens in handleUpdateRequest
// after the last chunk is processed.

static size_t otaFilesystemPartitionSize() {
    const esp_partition_t* part =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    return part ? part->size : 0;
}

// Restore the settings backed up before a filesystem image was flashed,
// verifying the write by reading the file back and parsing it as JSON.
// On success the verified config is also written to /config.bak so the
// layered config fallback keeps working on the fresh image. On any failure
// the partial file is removed so boot falls back to /config.bak, the
// factory template, then the NVS Wi-Fi credentials instead of choking on
// a corrupt /config.json.
static bool restoreOtaConfigBackup(const String& backup) {
    if (!LittleFS.begin()) {
        DEBUG_PRINTLN("OTA config restore: LittleFS remount failed");
        return false;
    }
    bool ok = false;
    // "w" truncates: the fresh image must not keep any factory config the
    // backup would otherwise be appended to.
    File f = LittleFS.open(CONFIG_JSON, "w");
    if (f) {
        ok = f.print(backup) == backup.length();
        f.close();
    }
    if (ok) {
        File r = LittleFS.open(CONFIG_JSON, "r");
        String written = r ? r.readString() : String();
        if (r) {
            r.close();
        }
        if (written != backup) {
            DEBUG_PRINTLN("OTA config restore: read-back mismatch");
            ok = false;
        } else {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, written);
            if (err != DeserializationError::Ok || !doc.is<JsonObject>()) {
                DEBUG_PRINTF("OTA config restore: backup is not valid JSON: %s\n", err.c_str());
                ok = false;
            }
        }
    } else {
        DEBUG_PRINTLN("OTA config restore: write failed");
    }
    if (!ok) {
        LittleFS.remove(CONFIG_JSON);
        return false;
    }
    // Re-establish the backup layer on the fresh image. A failure here is
    // non-fatal: the primary config is verified good.
    File b = LittleFS.open(CONFIG_JSON_BAK, "w");
    bool bakOk = false;
    if (b) {
        bakOk = b.print(backup) == backup.length();
        b.close();
    }
    if (!bakOk) {
        DEBUG_PRINTLN("OTA config restore: could not write /config.bak");
        LittleFS.remove(CONFIG_JSON_BAK);
    }
    return true;
}

void ServerManager_::handleUpdateUpload(AsyncWebServerRequest* request, const String& filename,
                                        size_t index, uint8_t* data, size_t len, bool final,
                                        int command) {
    if (index == 0) {
        otaUpdateCommand = command;
        otaUpdateAuthFailed = false;
        otaUpdateWritten = 0;
        otaUpdateError = "";
        // Non-sending check: the 401 (if any) goes out in handleUpdateRequest.
        if (!isRequestAuthenticated(request)) {
            otaUpdateAuthFailed = true;
            return;
        }
        if (command == U_FLASH) {
            // Reject a wrong-type file (e.g. a filesystem image) uploaded here.
            if (len == 0 || data[0] != 0xE9) {
                otaUpdateError = "not a firmware image";
                return;
            }
        } else if (!filename.endsWith(".bin")) {
            otaUpdateError = "filesystem image must be a .bin file";
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, command)) {
            otaUpdateError = Update.errorString();
            return;
        }
        if (command == U_SPIFFS) {
            // Back up the runtime settings before the image is replaced; the
            // uploaded image ships only factory defaults.
            otaConfigBackup = "";
            if (LittleFS.exists(CONFIG_JSON)) {
                File f = LittleFS.open(CONFIG_JSON, "r");
                if (f) {
                    otaConfigBackup = f.readString();
                    f.close();
                }
            }
            // Unmount the partition being overwritten so nothing reads it mid-write.
            LittleFS.end();
        }
        DEBUG_PRINTF("OTA %s update started: %s\n", command == U_FLASH ? "firmware" : "filesystem",
                     filename.c_str());
    }
    if (otaUpdateAuthFailed || otaUpdateError.length() > 0) {
        return;  // drain the remaining chunks without writing
    }
    if (len > 0) {
        if (Update.write(data, len) != len) {
            otaUpdateError = Update.errorString();
            return;
        }
        otaUpdateWritten += len;
    }
    if (final) {
        if (command == U_SPIFFS) {
            // mklittlefs images are always exactly the partition size; anything
            // else is the wrong file (e.g. a firmware image on this endpoint).
            size_t fsSize = otaFilesystemPartitionSize();
            if (fsSize == 0 || otaUpdateWritten != fsSize) {
                otaUpdateError = "filesystem image size mismatch";
                Update.abort();
                return;
            }
        }
        if (!Update.end(true)) {
            otaUpdateError = Update.errorString();
        } else {
            DEBUG_PRINTF("OTA %s update written: %u bytes\n", command == U_FLASH ? "firmware" : "filesystem",
                         otaUpdateWritten);
        }
    }
}

void ServerManager_::handleUpdateRequest(AsyncWebServerRequest* request) {
    if (!enforceAuthentication(request)) {
        return;
    }
    int command = otaUpdateCommand;
    String error = otaUpdateError;
    otaUpdateCommand = -1;
    otaUpdateError = "";

    if (command < 0) {
        request->send(400, "application/json", "{\"status\": \"error\", \"error\": \"no file uploaded\"}");
        return;
    }
    if (error.length() > 0) {
        String body = "{\"status\": \"error\", \"error\": \"";
        body += error;
        body += "\"}";
        request->send(400, "application/json", body);
        return;
    }
    // Restore the runtime settings into the new image BEFORE reporting success,
    // so the UI never claims success when the settings were actually lost.
    // A failed restore is reported as a warning: the update itself succeeded
    // and the clock reboots onto its config fallbacks (backup, factory, NVS).
    bool configOk = true;
    if (command == U_SPIFFS && otaConfigBackup.length() > 0) {
        configOk = restoreOtaConfigBackup(otaConfigBackup);
        otaConfigBackup = "";
        if (!configOk) {
            DEBUG_PRINTLN("OTA filesystem update: settings restore failed, using fallbacks");
        }
    }
    if (configOk) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
    } else {
        request->send(200, "application/json",
                      "{\"status\": \"ok\", \"warning\": \"settings could not be restored; "
                      "the clock will reboot with default settings\"}");
    }
    // Reboot into the new image after the response has been flushed.
    xTaskCreate(
        [](void*) {
            vTaskDelay(pdMS_TO_TICKS(1500));
            LittleFS.end();
            ESP.restart();
        },
        "ota_restart", 2048, NULL, 1, NULL);
}

// --- Self-update (pull) ----------------------------------------------------
// The clock fetches a small manifest from the release site, compares versions,
// and downloads the image itself. This works anywhere the clock has outbound
// internet (e.g. a school network), with no inbound connection needed.
// The manifest (www/update.json, published by the release workflow) carries
// SHA-256 hashes so a tampered or truncated download is rejected before the
// image is marked valid.

static const char* UPDATE_SITE_URL = "https://ktomy.github.io/nightscout-clock";
static const char* UPDATE_MANIFEST_URL = "https://ktomy.github.io/nightscout-clock/update.json";

bool ServerManager_::fetchUpdateManifest(String& body, String& error) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(client, UPDATE_MANIFEST_URL)) {
        error = "cannot start request";
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        error = "manifest request failed: " + String(code);
        http.end();
        return false;
    }
    body = http.getString();
    http.end();
    return true;
}

void ServerManager_::handleUpdateCheck(AsyncWebServerRequest* request) {
    if (!enforceAuthentication(request)) {
        return;
    }
    String manifest, error;
    if (!fetchUpdateManifest(manifest, error)) {
        request->send(502, "application/json",
                      "{\"status\": \"error\", \"error\": \"" + error + "\"}");
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, manifest) || !doc["version"].is<const char*>()) {
        request->send(502, "application/json",
                      "{\"status\": \"error\", \"error\": \"invalid manifest\"}");
        return;
    }
    String latest = doc["version"].as<const char*>();
    bool updateAvailable = latest.length() > 0 && latest != VERSION;
    String fsVersion = readFilesystemVersion();
    bool fsUpdateAvailable = latest.length() > 0 && latest != fsVersion;
    String body = "{\"status\": \"ok\", \"current\": \"" VERSION "\", \"latest\": \"";
    body += latest;
    body += "\", \"updateAvailable\": ";
    body += updateAvailable ? "true" : "false";
    body += ", \"fsCurrent\": \"";
    body += fsVersion;
    body += "\", \"fsUpdateAvailable\": ";
    body += fsUpdateAvailable ? "true" : "false";
    body += "}";
    request->send(200, "application/json", body);
}

void ServerManager_::handleUpdateApply(AsyncWebServerRequest* request) {
    if (!enforceAuthentication(request)) {
        return;
    }
    String type = request->hasParam("type") ? request->getParam("type")->value() : "";
    int command = type == "firmware" ? U_FLASH : type == "filesystem" ? U_SPIFFS : -1;
    if (command < 0) {
        request->send(400, "application/json",
                      "{\"status\": \"error\", \"error\": \"type must be firmware or filesystem\"}");
        return;
    }
    if (otaPullState == OtaPullState::DOWNLOADING || otaPullState == OtaPullState::VERIFYING ||
        otaUpdateCommand >= 0) {
        request->send(409, "application/json",
                      "{\"status\": \"error\", \"error\": \"an update is already in progress\"}");
        return;
    }
    String manifest, error;
    if (!fetchUpdateManifest(manifest, error)) {
        request->send(502, "application/json",
                      "{\"status\": \"error\", \"error\": \"" + error + "\"}");
        return;
    }
    JsonDocument doc;
    const char* key = command == U_FLASH ? "firmware" : "filesystem";
    if (deserializeJson(doc, manifest) || !doc[key]["url"].is<const char*>() ||
        !doc[key]["sha256"].is<const char*>()) {
        request->send(502, "application/json",
                      "{\"status\": \"error\", \"error\": \"invalid manifest\"}");
        return;
    }
    String url = String(UPDATE_SITE_URL) + "/" + doc[key]["url"].as<const char*>();
    String sha256 = doc[key]["sha256"].as<const char*>();
    String startError;
    if (!startOtaPull(command, url, sha256, true, startError)) {
        request->send(409, "application/json",
                      "{\"status\": \"error\", \"error\": \"" + startError + "\"}");
        return;
    }
    request->send(202, "application/json", "{\"status\": \"started\"}");
}

/**
 * Begin a pull update if none is running. The download task validates the
 * image and flashes it; on success it reboots unless rebootAfter is false.
 */
bool ServerManager_::startOtaPull(int command, const String& url, const String& sha256,
                                  bool rebootAfter, String& error) {
    if (otaPullState == OtaPullState::DOWNLOADING || otaPullState == OtaPullState::VERIFYING ||
        otaUpdateCommand >= 0) {
        error = "an update is already in progress";
        return false;
    }
    otaPullUrl = url;
    otaPullSha256 = sha256;
    otaPullCommand = command;
    otaPullReboot = rebootAfter;
    otaPullState = OtaPullState::DOWNLOADING;
    otaPullProgress = 0;
    otaPullError = "";
    xTaskCreate(otaPullTask, "ota_pull", 8192, this, 1, NULL);
    return true;
}

/**
 * The LittleFS image ships /version.txt, so the running firmware can tell which
 * filesystem release is flashed without trusting any cached state.
 */
String ServerManager_::readFilesystemVersion() {
    if (!LittleFS.exists("/version.txt")) {
        return "";
    }
    File f = LittleFS.open("/version.txt", "r");
    if (!f) {
        return "";
    }
    String v = f.readString();
    f.close();
    v.trim();
    return v;
}

void ServerManager_::handleUpdateStatus(AsyncWebServerRequest* request) {
    if (!enforceAuthentication(request)) {
        return;
    }
    const char* state = "idle";
    switch (otaPullState) {
        case OtaPullState::DOWNLOADING: state = "downloading"; break;
        case OtaPullState::VERIFYING: state = "verifying"; break;
        case OtaPullState::DONE: state = "done"; break;
        case OtaPullState::ERROR: state = "error"; break;
        case OtaPullState::IDLE: break;
    }
    String body = "{\"status\": \"ok\", \"state\": \"";
    body += state;
    body += "\", \"progress\": ";
    body += otaPullProgress;
    body += ", \"error\": \"";
    body += otaPullError;
    body += "\"}";
    request->send(200, "application/json", body);
}

static void sha256Hex(const uint8_t* hash, char* out) {
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        out[i * 2] = digits[hash[i] >> 4];
        out[i * 2 + 1] = digits[hash[i] & 0xF];
    }
    out[64] = '\0';
}

void ServerManager_::otaPullTask(void* param) {
    auto* self = static_cast<ServerManager_*>(param);
    String error;
    size_t totalWritten = 0;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(30000);
    if (!http.begin(client, self->otaPullUrl)) {
        error = "cannot start download";
    } else {
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            error = "download failed: " + String(code);
            http.end();
        } else {
            int total = http.getSize();
            if (self->otaPullCommand == U_SPIFFS) {
                // mklittlefs images are exactly the partition size; anything else
                // is the wrong file for this endpoint.
                size_t fsSize = otaFilesystemPartitionSize();
                if (total <= 0 || (size_t)total != fsSize) {
                    error = "filesystem image size mismatch";
                }
            }
            if (error.isEmpty() && !Update.begin(total > 0 ? (size_t)total : UPDATE_SIZE_UNKNOWN,
                                                 self->otaPullCommand)) {
                error = Update.errorString();
            }
        if (error.isEmpty()) {
            // A filesystem update writes the partition the web UI is served
            // from. Back up the runtime settings first: the new image ships
            // only factory defaults, so without this the clock would lose
            // WiFi and the selected face on reboot.
            if (self->otaPullCommand == U_SPIFFS) {
                self->otaConfigBackup = "";
                if (LittleFS.exists(CONFIG_JSON)) {
                    File f = LittleFS.open(CONFIG_JSON, "r");
                    if (f) {
                        self->otaConfigBackup = f.readString();
                        f.close();
                    }
                }
                LittleFS.end();
            }
            mbedtls_sha256_context sha;
            mbedtls_sha256_init(&sha);
            mbedtls_sha256_starts(&sha, 0);
            WiFiClient* stream = http.getStreamPtr();
            uint8_t buf[1024];
            bool firstChunk = true;
            unsigned long lastReadMs = millis();
            while (error.isEmpty() && (total <= 0 || totalWritten < (size_t)total)) {
                size_t avail = stream->available();
                if (avail > 0) {
                    size_t n = stream->readBytes(buf, avail > sizeof(buf) ? sizeof(buf) : avail);
                    if (n == 0) {
                        break;
                    }
                    if (firstChunk && self->otaPullCommand == U_FLASH && buf[0] != 0xE9) {
                        error = "not a firmware image";
                        break;
                    }
                    firstChunk = false;
                    mbedtls_sha256_update(&sha, buf, n);
                    if (Update.write(buf, n) != n) {
                        error = Update.errorString();
                        break;
                    }
                    totalWritten += n;
                    lastReadMs = millis();
                    if (total > 0) {
                        self->otaPullProgress = (int)(totalWritten * 100 / (size_t)total);
                    }
                } else if (!stream->connected()) {
                    break;
                } else {
                    if (millis() - lastReadMs > 60000) {
                        error = "download stalled";
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            }
            uint8_t hash[32];
            mbedtls_sha256_finish(&sha, hash);
            mbedtls_sha256_free(&sha);
            if (error.isEmpty()) {
                if (total > 0 && totalWritten != (size_t)total) {
                    error = "truncated download";
                } else {
                    char hex[65];
                    sha256Hex(hash, hex);
                    self->otaPullState = OtaPullState::VERIFYING;
                    if (self->otaPullSha256.equalsIgnoreCase(hex)) {
                        if (Update.end(true)) {
                            if (self->otaPullCommand == U_SPIFFS) {
                                // Restore the runtime settings into the new image so the
                                // clock comes back on the same network and face.
                                // Verified: a failed restore removes the partial file so
                                // boot falls back cleanly.
                                if (self->otaConfigBackup.length() > 0 &&
                                    !restoreOtaConfigBackup(self->otaConfigBackup)) {
                                    DEBUG_PRINTLN("OTA pull: settings restore failed, using fallbacks");
                                }
                                self->otaConfigBackup = "";
                            }
                            DEBUG_PRINTF("OTA pull %s complete: %u bytes\n",
                                         self->otaPullCommand == U_FLASH ? "firmware" : "filesystem",
                                         totalWritten);
                        } else {
                            error = Update.errorString();
                        }
                    } else {
                        error = "checksum mismatch";
                    }
                }
            }
            if (!error.isEmpty()) {
                Update.abort();
            }
        }
        http.end();
        }
    }

    if (error.isEmpty()) {
        self->otaPullProgress = 100;
        self->otaPullState = OtaPullState::DONE;
        if (self->otaPullReboot) {
            // Let the status poll observe "done" before rebooting.
            vTaskDelay(pdMS_TO_TICKS(1500));
            ESP.restart();
        }
    } else {
        self->otaPullError = error;
        self->otaPullState = OtaPullState::ERROR;
        DEBUG_PRINTF("OTA pull failed: %s\n", error.c_str());
    }
    vTaskDelete(NULL);
}

IPAddress ServerManager_::startWifi() {
    this->isInAPMode = false;

    IPAddress ip;

    bool connected = tryConnectToWiFi(
        "wpa_psk", SettingsManager.settings.ssid, "", SettingsManager.settings.wifi_password);
    if (!connected && SettingsManager.settings.additional_wifi_enable) {
        connected = tryConnectToWiFi(
            SettingsManager.settings.additional_wifi_type, SettingsManager.settings.additional_wifi_ssid,
            SettingsManager.settings.additional_wifi_username,
            SettingsManager.settings.additional_wifi_password);
    }

    if (connected) {
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);
        ip = WiFi.localIP();
        DEBUG_PRINTLN("Connected");
        failedAttempts = 0;  // Reset failed API call attempts counter
        return ip;
    }

    DEBUG_PRINTLN("Failed to connect to WiFi, starting AP mode");

    ip = setAPmode(getHostname(), AP_MODE_PASSWORD);
    this->isInAPMode = true;
    WiFi.begin();
    return ip;
}

// When we cannot get BG for some time, we want to reconnect to wifi
void ServerManager_::reconnectWifi() {
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    myIP = startWifi();
    failedAttempts = 0;
}

AsyncWebHandler ServerManager_::addHandler(AsyncWebHandler* handler) {
    if (ws != nullptr) {
        ws->addHandler(handler);
    }
    return *handler;
}

bool canReachInternet() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return false;
    }

    WiFiClient client;
    const char* host = "www.google.com";
    const uint16_t port = 80;
    if (!client.connect(host, port, 2000)) {
        DEBUG_PRINTLN("Internet not reachable (connect failed)");
        client.stop();
        return false;
    }

    // Send a simple HTTP GET request
    client.print("GET /generate_204 HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");

    unsigned long start = millis();
    while (client.connected() && !client.available() && millis() - start < 2000) {
        delay(10);
    }

    if (!client.available()) {
        DEBUG_PRINTLN("Internet not reachable (no response)");
        client.stop();
        return false;
    }

    // Optionally, check for HTTP/1.1 204 No Content response
    String line = client.readStringUntil('\n');
    if (line.indexOf("204") > 0 || line.indexOf("200") > 0) {
        // DEBUG_PRINTLN("Internet reachable");
        client.stop();
        return true;
    }

    DEBUG_PRINTLN("Internet not reachable (bad response)");
    client.stop();
    return false;
}

void ServerManager_::setupWebServer(IPAddress ip) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("ServerManager::setupWebServer");
#endif
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->on("/api/auth/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"enabled\": ";
        jsonResponse += isWebAuthEnabled() ? "true" : "false";
        jsonResponse += ", \"authenticated\": ";
        jsonResponse += isRequestAuthenticated(request) ? "true" : "false";
        jsonResponse += ", \"hasPassword\": ";
        jsonResponse += SettingsManager.settings.web_auth_password.length() > 0 ? "true" : "false";
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/auth/login", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            if (!isWebAuthEnabled()) {
                request->send(200, "application/json", "{\"status\": \"disabled\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            String password = data["password"].as<String>();
            if (password != SettingsManager.settings.web_auth_password) {
                request->send(401, "application/json", "{\"status\": \"invalid\"}");
                return;
            }

            webAuthToken = generateAuthToken();
            webAuthTokenIssuedMs = millis();
            String jsonResponse = "{\"status\": \"ok\"}";
            auto response = request->beginResponse(200, "application/json", jsonResponse);
            response->addHeader(
                "Set-Cookie", buildAuthCookie(webAuthToken, WEB_AUTH_COOKIE_MAX_AGE_SEC));
            request->send(response);
        }));

    ws->on("/api/auth/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!isWebAuthEnabled()) {
            request->send(200, "application/json", "{\"status\": \"disabled\"}");
            return;
        }
        if (!isRequestAuthenticated(request)) {
            request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
            return;
        }
        webAuthToken = "";
        webAuthTokenIssuedMs = 0;
        auto response = request->beginResponse(200, "application/json", "{\"status\": \"ok\"}");
        response->addHeader("Set-Cookie", buildAuthCookie("", 0));
        request->send(response);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/save", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(200, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            auto sendSaveValidationError = [request](const char* error) {
                String response = "{\"status\": \"error\", \"error\": \"";
                response += error;
                response += "\"}";
                request->send(400, "application/json", response);
            };

            if (!data["face_cycle_enabled"].isNull() && !data["face_cycle_enabled"].is<bool>()) {
                sendSaveValidationError("face_cycle_enabled must be a boolean");
                return;
            }

            bool faceCycleEnabled = data["face_cycle_enabled"] | false;
            bool hasInactiveFaces = !data["inactive_faces"].isNull();
            int inactiveFaceCount = 0;
            if (hasInactiveFaces) {
                if (!data["inactive_faces"].is<JsonArray>()) {
                    sendSaveValidationError("inactive_faces must be an array");
                    return;
                }

                bool inactiveFaces[CLOCK_FACE_COUNT] = {};
                for (JsonVariant face : data["inactive_faces"].as<JsonArray>()) {
                    if (!face.is<int>()) {
                        sendSaveValidationError("Face selections must use valid clock face IDs");
                        return;
                    }

                    int faceId = face.as<int>();
                    if (faceId < 0 || faceId >= CLOCK_FACE_COUNT) {
                        sendSaveValidationError("Face selections must use valid clock face IDs");
                        return;
                    }

                    if (!inactiveFaces[faceId]) {
                        inactiveFaces[faceId] = true;
                        inactiveFaceCount++;
                    }
                }
            }

            if (faceCycleEnabled && CLOCK_FACE_COUNT - inactiveFaceCount < 2) {
                sendSaveValidationError("Cycling needs at least two active clock faces");
                return;
            }

            bool hasFaceCycleInterval = !data["face_cycle_interval_seconds"].isNull();
            if (faceCycleEnabled || hasFaceCycleInterval) {
                if (!data["face_cycle_interval_seconds"].is<int>()) {
                    sendSaveValidationError(
                        "Face cycle period must be 10, 30, 60, 120, 180, or 300 seconds");
                    return;
                }

                int intervalSeconds = data["face_cycle_interval_seconds"].as<int>();
                if (intervalSeconds != 10 && intervalSeconds != 30 && intervalSeconds != 60 &&
                    intervalSeconds != 120 && intervalSeconds != 180 && intervalSeconds != 300) {
                    sendSaveValidationError(
                        "Face cycle period must be 10, 30, 60, 120, 180, or 300 seconds");
                    return;
                }
            }

            // Refuse a value the loader would silently correct; the accepted set lives in SettingsManager.
            if (!data["alarm_repeat_interval_seconds"].isNull()) {
                if (!data["alarm_repeat_interval_seconds"].is<int>() ||
                    !SettingsManager_::isValidAlarmRepeatInterval(
                        data["alarm_repeat_interval_seconds"].as<int>())) {
                    sendSaveValidationError(
                        "Alarm repeat interval must be 60, 120, or 300 seconds");
                    return;
                }
            }

            if (SettingsManager.trySaveJsonAsSettings(data)) {
                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(200, "application/json", "{\"status\": \"Settings save error\"}");
            }
        }));

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/alarm", [](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            if (!data["rtttl"].is<String>()) {
                request->send(400, "application/json", "{\"status\": \"rtttl key not found\"}");
                return;
            }

            auto melody = data["rtttl"].as<String>();
            melody.trim();

            if (melody.length() == 0 || melody.indexOf(':') < 0) {
                request->send(400, "application/json", "{\"status\": \"invalid melody\"}");
                return;
            }

            PeripheryManager.playRTTTLString(melody);
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        }));

    ws->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        LittleFS.end();
        ESP.restart();
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/displaypower", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (data["power"].is<String>()) {
                auto powerState = data["power"].as<String>();
                if (powerState == "on") {
                    DisplayManager.setPower(true);
                } else if (powerState == "off") {
                    DisplayManager.setPower(false);
                } else {
                    request->send(400, "application/json", "{\"status\": \"Invalid power state\"}");
                    return;
                }

                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\": \"power key not found\"}");
            }
        }));

    ws->on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        SettingsManager.factoryReset();
    });

    // Authenticated network OTA updates. Firmware and the LittleFS filesystem
    // image are distinct update types with separate endpoints and validation;
    // see handleUpdateUpload. Requires the dual-OTA partition layout.
    ws->on(
        "/api/update/firmware", HTTP_POST,
        [this](AsyncWebServerRequest* request) { handleUpdateRequest(request); },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data,
               size_t len, bool final) {
            handleUpdateUpload(request, filename, index, data, len, final, U_FLASH);
        });
    ws->on(
        "/api/update/filesystem", HTTP_POST,
        [this](AsyncWebServerRequest* request) { handleUpdateRequest(request); },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data,
               size_t len, bool final) {
            handleUpdateUpload(request, filename, index, data, len, final, U_SPIFFS);
        });

    // Self-update (pull): the clock downloads release images from the project
    // site itself. Check compares versions; apply starts a background download
    // task; status reports progress. Works wherever the clock has outbound
    // internet, with no inbound connection needed.
    ws->on("/api/update/check", HTTP_POST,
           [this](AsyncWebServerRequest* request) { handleUpdateCheck(request); });
    ws->on("/api/update/apply", HTTP_POST,
           [this](AsyncWebServerRequest* request) { handleUpdateApply(request); });
    ws->on("/api/update/status", HTTP_GET,
           [this](AsyncWebServerRequest* request) { handleUpdateStatus(request); });

    // api call which returns status (isConnected, internet is reacheable, is in AP mode, bg source type
    // and status)
    ws->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"isConnected\": ";
        jsonResponse += this->isConnected ? "true" : "false";
        jsonResponse += ", \"hasInternet\": ";
        jsonResponse += canReachInternet() ? "true" : "false";
        jsonResponse += ", \"isInAPMode\": ";
        jsonResponse += this->isInAPMode ? "true" : "false";
        jsonResponse += ", \"bgSource\": \"";
        jsonResponse += toString(bgSourceManager.getCurrentSourceType());
        jsonResponse += "\", \"bgSourceStatus\": \"";
        jsonResponse += bgSourceManager.getSourceStatus();
        jsonResponse += "\", \"sgv\": ";
        auto glucoseData = bgSourceManager.getGlucoseData();
        if (!glucoseData.empty()) {
            jsonResponse += String(glucoseData.back().sgv);
        } else {
            jsonResponse += "0";
        }
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    // Manual heartbeat test ping: POST an optional {"url"} to ping that URL
    // (handy for testing the address typed in the web UI before saving it),
    // otherwise the saved heartbeat URL. The ping runs in the background;
    // "ok" means it was started, and delivery is confirmed at the receiver.
    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/heartbeat/test", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            String url;
            if (json.is<JsonObject>()) {
                url = json.as<JsonObject>()["url"].as<String>();
            }
            url.trim();
            if (url.length() == 0) {
                url = SettingsManager.settings.healthcheck_url;
                url.trim();
            }
            if (url.length() == 0) {
                request->send(400, "application/json",
                              "{\"status\": \"error\", \"error\": \"no heartbeat URL configured\"}");
                return;
            }
            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                request->send(400, "application/json",
                              "{\"status\": \"error\", \"error\": \"URL must start with http:// or https://\"}");
                return;
            }
            sendHeartbeatNow(url);
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        }));

    ws->on("/config.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(LittleFS, CONFIG_JSON, "application/json");
    });

    addStaticFileHandler();

    // Captive portal detection endpoints
    // Android: responds with HTTP 204 so the device opens the portal
    ws->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    // Windows: redirect NCSI probe to the settings page
    ws->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });

    ws->onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    ws->begin();
}

void ServerManager_::removeStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        ws->removeHandler(staticFilesHandler);
        staticFilesHandler = nullptr;
    } else {
        DEBUG_PRINTLN("removeStaticFileHandler: staticFilesHandler is null");
    }
}

void ServerManager_::addStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        removeStaticFileHandler();
    }
    staticFilesHandler = new AsyncStaticWebHandler("/", LittleFS, "/", NULL);
    staticFilesHandler->setDefaultFile("index.html");
    ws->addHandler(staticFilesHandler);
}

bool ServerManager_::initTimeIfNeeded() {
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo) || getUtcEpoch() - lastTimeSync > TIME_SYNC_INTERVAL) {
        configTime(0, 0, "pool.ntp.org");  // Connect to NTP server, with 0 TZ offset
        if (!getLocalTime(&timeinfo)) {
            DEBUG_PRINTLN("Failed to obtain time");
            return false;
        }

        lastTimeSync = getUtcEpoch();

        setTimezone();

        timeinfo = getTimezonedTime();

        DEBUG_PRINTF(
            "Timezone is: %s, local time is: %02d.%02d.%d %02d:%02d:%02d\n",
            SettingsManager.settings.tz_libc_value.c_str(), timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        Serial.printf(
            "Local time is: %02d.%02d.%d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return true;
}

void ServerManager_::setTimezone() {
    setenv("TZ", SettingsManager.settings.tz_libc_value.c_str(), 1);
    tzset();
}

unsigned long ServerManager_::getUtcEpoch() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
        return 0;
    } else {
        auto utc = time(nullptr);
        return utc;
    }
}

tm ServerManager_::getTimezonedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
    }
    return timeinfo;
}

// Like getTimezonedTime(), but reports whether the clock actually knows the time. Reads the
// clock directly: getLocalTime() waits up to 5 s when it is unset, and can skip the read on a zero timeout.
bool ServerManager_::tryGetTimezonedTime(tm& timeinfo) {
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo.tm_year > (2016 - 1900);
}

void ServerManager_::stop() {
    ws->end();
    delete ws;
    WiFi.disconnect();
}

void ServerManager_::setup() {
    WiFi.setHostname(getHostname().c_str());  // define hostname

    myIP = startWifi();

    auto ipAP = IPAddress();
    ipAP.fromString(AP_IP);
    DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);

    setupWebServer(myIP);
    setTimezone();

    isConnected = myIP != ipAP;
}

void ServerManager_::tick() {
    if (apMode) {
        dnsServer.processNextRequest();
    } else {
        initTimeIfNeeded();
        tickAutoUpdate();
        tickHeartbeat();
    }
}

/**
 * Automatic self-update: once shortly after boot (catches anything missed
 * while offline) and then once a day at the configured local hour, the clock
 * checks the release manifest and applies new images itself. The filesystem
 * image goes first when both are outdated, so a single reboot lands both.
 */
void ServerManager_::tickAutoUpdate() {
    if (!SettingsManager.settings.ota_auto_update || WiFi.status() != WL_CONNECTED) {
        return;
    }
    if (otaAutoBootMs == 0) {
        otaAutoBootMs = millis();
    }
    bool due = false;
    bool bootCheck = false;
    if (!otaAutoBootCheckDone) {
        bootCheck = true;
        due = millis() - otaAutoBootMs > 180000;
    } else if (time(nullptr) > 1700000000) {
        tm t = getTimezonedTime();
        int hour = SettingsManager.settings.ota_auto_update_hour;
        if (hour < 0) {
            hour = 0;
        } else if (hour > 23) {
            hour = 23;
        }
        due = t.tm_hour == hour && otaAutoLastCheckYday != t.tm_yday;
        if (due) {
            otaAutoLastCheckYday = t.tm_yday;
        }
    }
    if (!due) {
        return;
    }
    if (bootCheck) {
        otaAutoBootCheckDone = true;
    }
    String manifest, error;
    if (!fetchUpdateManifest(manifest, error)) {
        DEBUG_PRINTF("Auto-update check failed: %s\n", error.c_str());
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, manifest) || !doc["version"].is<const char*>()) {
        DEBUG_PRINTLN("Auto-update check failed: bad manifest");
        return;
    }
    String latest = doc["version"].as<const char*>();
    const char* key = nullptr;
    int command = -1;
    if (latest.length() > 0 && latest != readFilesystemVersion() && doc["filesystem"]["url"]) {
        key = "filesystem";
        command = U_SPIFFS;
    } else if (latest.length() > 0 && latest != VERSION && doc["firmware"]["url"]) {
        key = "firmware";
        command = U_FLASH;
    }
    if (command < 0) {
        return;  // up to date
    }
    String url = String(UPDATE_SITE_URL) + "/" + doc[key]["url"].as<const char*>();
    String sha256 = doc[key]["sha256"].as<const char*>();
    if (startOtaPull(command, url, sha256, true, error)) {
        DEBUG_PRINTF("Auto-update started: %s %s\n", key, latest.c_str());
    } else {
        DEBUG_PRINTF("Auto-update not started: %s\n", error.c_str());
    }
}

/**
 * Map BG_SOURCE to a short human-readable name for the heartbeat payload.
 */
static const char* bgSourceName(BG_SOURCE source) {
    switch (source) {
        case BG_SOURCE::NIGHTSCOUT: return "nightscout";
        case BG_SOURCE::DEXCOM: return "dexcom";
        case BG_SOURCE::MEDTRONIC: return "medtronic";
        case BG_SOURCE::API: return "api";
        case BG_SOURCE::LIBRELINKUP: return "librelinkup";
        case BG_SOURCE::MEDTRUM: return "medtrum";
        case BG_SOURCE::NO_SOURCE:
        default: return "none";
    }
}

/**
 * Status heartbeat: POST a small JSON blob to the configured URL on a
 * schedule so the clock can be watched from anywhere. Works with
 * healthchecks.io (dead-man's-switch alerting), ntfy.sh (phone push), or
 * any webhook receiver.
 */
void ServerManager_::tickHeartbeat() {
    String url = SettingsManager.settings.healthcheck_url;
    url.trim();
    if (url.length() == 0 || WiFi.status() != WL_CONNECTED) {
        return;
    }
    unsigned long now = millis();
    if (healthcheckLastMs == 0) {
        healthcheckLastMs = now;
    }
    int intervalH = SettingsManager.settings.healthcheck_interval_hours;
    if (intervalH < 1) {
        intervalH = 1;
    } else if (intervalH > 168) {
        intervalH = 168;
    }
    bool bootDue = !healthcheckBootSent && now > 120000;
    bool intervalDue = now - healthcheckLastMs >= (unsigned long)intervalH * 3600000UL;
    if (!bootDue && !intervalDue) {
        return;
    }
    if (bootDue) {
        healthcheckBootSent = true;
    }
    healthcheckLastMs = now;
    sendHeartbeatNow(SettingsManager.settings.healthcheck_url);
}

void ServerManager_::sendHeartbeatNow(const String& url) {
    // The task takes ownership of the heap-allocated URL and deletes it.
    xTaskCreate(heartbeatTask, "heartbeat", 8192, new String(url), 1, NULL);
}

void ServerManager_::heartbeatTask(void* param) {
    String* urlPtr = static_cast<String*>(param);
    String url = urlPtr ? *urlPtr : String();
    delete urlPtr;
    url.trim();
    if (url.length() == 0) {
        vTaskDelete(NULL);
        return;
    }
    JsonDocument doc;
    doc["version"] = VERSION;
    doc["uptime_s"] = (int)(millis() / 1000);
    doc["rssi_dbm"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["face"] = SettingsManager.settings.default_clockface;
    doc["display_on"] = !MATRIX_OFF;
    doc["heap_free"] = (int)ESP.getFreeHeap();
    // Battery (BATTERY_PERCENT/BATTERY_RAW are refreshed by PeripheryManager).
    doc["battery_pct"] = BATTERY_PERCENT;
    doc["battery_raw"] = BATTERY_RAW;
    // BG data freshness: the most useful remote diagnostic on a BG clock.
    doc["bg_source"] = bgSourceName(bgSourceManager.getCurrentSourceType());
    doc["bg_status"] = bgSourceManager.getSourceStatus();
    GlucoseReading* lastReading = bgDisplayManager.getLastDisplayedGlucoseReading();
    if (lastReading != NULL && lastReading->epoch > 0) {
        long long age = (long long)ServerManager.getUtcEpoch() - (long long)lastReading->epoch;
        doc["bg_sgv"] = lastReading->sgv;
        doc["bg_age_s"] = (int)(age > 0 ? age : 0);
    } else {
        doc["bg_age_s"] = -1;
    }
    String body;
    serializeJson(doc, body);
    bool useTls = url.startsWith("https://");
    WiFiClient* client = useTls ? new WiFiClientSecure() : new WiFiClient();
    if (useTls) {
        static_cast<WiFiClientSecure*>(client)->setInsecure();
    }
    HTTPClient http;
    http.setTimeout(15000);
    if (http.begin(*client, url)) {
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(body);
        DEBUG_PRINTF("Heartbeat -> %s: HTTP %d\n", url.c_str(), code);
        http.end();
    } else {
        DEBUG_PRINTF("Heartbeat -> %s: begin failed\n", url.c_str());
    }
    delete client;
    vTaskDelete(NULL);
}
