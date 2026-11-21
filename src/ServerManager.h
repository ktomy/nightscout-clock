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
    ServerManager_() = default;
    unsigned long lastTimeSync = 0;

    IPAddress startWifi();
    void setupWebServer(IPAddress ip);
    void addEmbeddedFileRoutes();
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
    unsigned long wifiScanStartedMs = 0;

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
    bool enforceAuthentication(AsyncWebServerRequest* request);
    // Whether the request may change settings: no web password, or a valid login. Unlike
    // enforceAuthentication() it sends no answer, for handlers that answer later.
    bool isAuthorized(AsyncWebServerRequest* request) const;
    int failedAttempts = 0;
    void reconnectWifi();
};

extern ServerManager_& ServerManager;

#endif
