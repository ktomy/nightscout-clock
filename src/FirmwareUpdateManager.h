#ifndef FIRMWAREUPDATEMANAGER_H
#define FIRMWAREUPDATEMANAGER_H

#include <Arduino.h>

#include <atomic>

class AsyncWebServer;
class AsyncWebServerRequest;

// Installs firmware uploaded from the settings page into the slot that is not running, and boots the
// other slot when asked to go back. See /api/update* in addRoutes().
class FirmwareUpdateManager_ {
public:
    static FirmwareUpdateManager_& getInstance();

    // Call at the end of setup(): looks in the other slot for the firmware an update replaced, and
    // from then on confirms the running firmware before any restart the firmware itself makes.
    void setup();
    void addRoutes(AsyncWebServer* ws);
    // Runs the update from the main loop: stops the data source, shows the progress, and restarts.
    // Outside an update it confirms a newly installed firmware once it has run for a minute.
    void tick();
    // True from the start request until the restart. The main loop then runs only the server and
    // this, so the display shows the update and nothing else uses the network.
    bool isActive() const { return state != State::IDLE; }
    // For code that never returns to the main loop, such as the fatal-error screen: once an update
    // has started, runs it here, and does not return, because it ends with a restart.
    void takeOverIfStarted();
    // Tells the bootloader the running firmware works, so a restart does not return to the previous
    // one. tick() does this after a minute, and setup() makes every restart after boot do it, so only
    // a crash in the first minute, or a restart during setup(), goes back.
    void confirmRunningFirmware();

private:
    enum class State : uint8_t { IDLE, STOPPING, READY, RECEIVING, SUCCEEDED, FAILED };

    FirmwareUpdateManager_() = default;
    FirmwareUpdateManager_(const FirmwareUpdateManager_&) = delete;
    FirmwareUpdateManager_& operator=(const FirmwareUpdateManager_&) = delete;

    void findPreviousFirmware();
    void handleUpload(
        AsyncWebServerRequest* request, size_t index, uint8_t* data, size_t len, bool final);
    void finishUploadRequest(AsyncWebServerRequest* request);
    // Both texts are constants, so the web server task and the main loop can set them without a lock.
    void fail(const char* reason, const char* detail = nullptr);
    // For the main loop's timeouts: fails only if the state is still `current`.
    void failIfStill(State current, const char* reason);
    String failureText() const;
    void showMessage(const char* text);
    static const char* stateName(State state);

    std::atomic<State> state{State::IDLE};
    unsigned long readySinceMs = 0;
    std::atomic<unsigned long> lastDataMs{0};
    // The upload being installed. Only compared, never used after its request ends.
    AsyncWebServerRequest* uploadRequest = nullptr;
    size_t markerMatched = 0;
    bool markerFound = false;
    std::atomic<const char*> failureReason{nullptr};
    std::atomic<const char*> failureDetail{nullptr};

    std::atomic<bool> confirmed{false};
    // Set after going back, until the restart a timer makes once the answer is out.
    std::atomic<bool> goingBack{false};

    // The firmware in the other slot, found by setup(). One that failed to start is named but not
    // available.
    bool previousAvailable = false;
    bool previousFailed = false;
    String previousVersion;
    bool previousDebug = false;
};

extern FirmwareUpdateManager_& FirmwareUpdateManager;

#endif
