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
    void begin();
    static ServerManager_ &getInstance();
    void setup();
    void tick();
    bool isConnected;
    IPAddress myIP;
    DNSServer dnsServer;

};

extern ServerManager_ &ServerManager;
 
#endif