#ifndef ServerManager_h
#define ServerManager_h

#include <Arduino.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class ServerManager_
{
private:
    bool apMode;
    AsyncWebServer *ws;
    ServerManager_() = default;

    IPAddress startWifi(String ssid, String password);
    void setupWebServer(IPAddress ip);
    IPAddress setAPmode(String ssid, String psk);
    void saveConfigHandler();

public:
    static ServerManager_ &getInstance();
    void setup();
    void tick();
    void stop();
    bool isConnected;
    bool isInAPMode;
    IPAddress myIP;
    DNSServer dnsServer;
    unsigned long getTime();
};

extern ServerManager_ &ServerManager;

#endif