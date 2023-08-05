#ifndef ServerManager_h
#define ServerManager_h

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

class ServerManager_
{
private:
    ServerManager_() = default;
    IPAddress startWifi(String ssid, String password);
    void setupWebServer(IPAddress ip);
    IPAddress setAPmode(String ssid, String psk);
    bool apMode;
    WebServer *ws;

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