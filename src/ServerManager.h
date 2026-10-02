#ifndef ServerManager_h
#define ServerManager_h

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

class ServerManager_ {
private:
    bool apMode;
    AsyncWebServer* ws;
    AsyncStaticWebHandler* staticFilesHandler = nullptr;
    ServerManager_() = default;
    unsigned long lastTimeSync = 0;

    IPAddress startWifi();
    void setupWebServer(IPAddress ip);
    IPAddress setAPmode(String ssid, String psk);
    void saveConfigHandler();
    bool initTimeIfNeeded();
    void setTimezone();
    String getHostname();
    bool isWebAuthEnabled() const;
    bool isRequestAuthenticated(AsyncWebServerRequest* request) const;
    String generateAuthToken();
    String webAuthToken;
    unsigned long webAuthTokenIssuedMs = 0;

    // Authenticated network OTA update state (POST /api/update/firmware and
    // POST /api/update/filesystem stream uploads into the Arduino Update class)
    int otaUpdateCommand = -1;
    // Runtime /config.json stashed before a filesystem image is flashed, then
    // restored into the new image so settings survive the update.
    String otaConfigBackup;
    bool otaUpdateAuthFailed = false;
    size_t otaUpdateWritten = 0;
    String otaUpdateError;
    void handleUpdateUpload(AsyncWebServerRequest* request, const String& filename, size_t index,
                            uint8_t* data, size_t len, bool final, int command);
    void handleUpdateRequest(AsyncWebServerRequest* request);

    // Self-update (pull): the clock downloads release images from the project
    // site itself. Only one update (push or pull) may run at a time.
    enum class OtaPullState : uint8_t { IDLE, DOWNLOADING, VERIFYING, DONE, ERROR };
    OtaPullState otaPullState = OtaPullState::IDLE;
    int otaPullProgress = 0;  // 0-100 while downloading
    String otaPullError;
    int otaPullCommand = 0;  // U_FLASH; Update.h is not included here
    String otaPullUrl;
    String otaPullSha256;
    void handleUpdateCheck(AsyncWebServerRequest* request);
    void handleUpdateApply(AsyncWebServerRequest* request);
    void handleUpdateStatus(AsyncWebServerRequest* request);
    bool fetchUpdateManifest(String& body, String& error);
    static void otaPullTask(void* param);
    // Start a pull update; rebootAfter=false leaves the new image staged without
    // rebooting (used to flash the filesystem before the firmware in one cycle).
    bool startOtaPull(int command, const String& url, const String& sha256, bool rebootAfter,
                      String& error);
    // Read the version baked into the running LittleFS image (/version.txt).
    String readFilesystemVersion();
    // tick()-driven automation: daily self-update check and status heartbeat.
    void tickAutoUpdate();
    void tickHeartbeat();
    // Fire off one heartbeat ping to the given URL (empty = no-op).
    void sendHeartbeatNow(const String& url);
    static void heartbeatTask(void* param);
    unsigned long otaAutoBootMs = 0;
    bool otaAutoBootCheckDone = false;
    int otaAutoLastCheckYday = -1;
    bool otaPullReboot = true;
    unsigned long healthcheckLastMs = 0;
    bool healthcheckBootSent = false;

public:
    static ServerManager_& getInstance();
    void setup();
    void tick();
    void stop();
    bool isConnected;
    bool isInAPMode;
    IPAddress myIP;
    DNSServer dnsServer;
    unsigned long getUtcEpoch();
    tm getTimezonedTime();
    bool tryGetTimezonedTime(tm& timeinfo);
    AsyncWebHandler addHandler(AsyncWebHandler* handler);
    void removeStaticFileHandler();
    void addStaticFileHandler();
    bool enforceAuthentication(AsyncWebServerRequest* request);
    int failedAttempts = 0;
    void reconnectWifi();
};

extern ServerManager_& ServerManager;

#endif