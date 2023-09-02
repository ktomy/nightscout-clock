#ifndef NightscoutManager_h
#define NightscoutManager_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class NightscoutManager_
{
private:
    HTTPClient *client;
    WiFiClientSecure *transportClient;
    void getBG(String server, int port, int numberOfvalues);

public:
    static NightscoutManager_ &getInstance();
    void setup();
    void tick();

};

extern NightscoutManager_ &NightscoutManager;
 
#endif