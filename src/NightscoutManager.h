#ifndef NightscoutManager_h
#define NightscoutManager_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <list>

#include "enums.h"
#include "DisplayManager.h"

struct GlucoseReading
{
public:
    int sgv;
    BG_TREND trend;
    // int secondsAgo;
    unsigned long long epoch;
    int getSecondsAgo()
    {
        return time(NULL) - epoch;
    }
    String toString() const
    {
        return String(sgv) + "," + String(trend) + "," + String(epoch);
    }
};

class NightscoutManager_
{
private:
    HTTPClient *client;
    WiFiClientSecure *wifiSecureClient;
    WiFiClient *wifiClient;
    void getBG(String baseUrl, int numberOfvalues);
    unsigned long lastReadingEpoch;
    std::list<GlucoseReading> glucoseReadings;
    bool firstConnectionSuccess;

public:
    static NightscoutManager_ &getInstance();
    void setup();
    void tick();
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();
};

extern NightscoutManager_ &NightscoutManager;

#endif