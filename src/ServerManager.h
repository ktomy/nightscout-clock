#ifndef ServerManager_h
#define ServerManager_h

#include <Arduino.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class ServerManager_ {
  private:
    bool apMode;
    AsyncWebServer *ws;
    AsyncStaticWebHandler *staticFilesHandler = nullptr;
    ServerManager_() = default;
    unsigned long lastTimeSync = 0;

    IPAddress startWifi(String ssid, String password);
    void setupWebServer(IPAddress ip);
    IPAddress setAPmode(String ssid, String psk);
    void saveConfigHandler();
    bool initTimeIfNeeded();
    void setTimezone();

  public:
    static ServerManager_ &getInstance();
    void setup();
    void tick();
    void stop();
    bool isConnected;
    bool isInAPMode;
    IPAddress myIP;
    DNSServer dnsServer;
    unsigned long getUtcEpoch();
    tm getTimezonedTime();
    AsyncWebHandler addHandler(AsyncWebHandler *handler);
    void removeStaticFileHandler();
    void addStaticFileHandler();
    int failedAttempts = 0;
    void reconnectWifi();
};

extern ServerManager_ &ServerManager;

#endif