#ifndef NightscoutManager_h
#define NightscoutManager_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <list>

#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "enums.h"

class NightscoutManager_ {
  private:
    HTTPClient *client;
    WiFiClientSecure *wifiSecureClient;
    WiFiClient *wifiClient;
    // unsigned long lastReadingEpoch;
    std::list<GlucoseReading> glucoseReadings;
    bool firstConnectionSuccess;
    std::list<GlucoseReading> updateReadings(String baseUrl, String apiKey, std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(String baseUrl, String apiKey, unsigned long long lastReadingEpoch,
                                               unsigned long long readingToEpoch, int numberOfvalues);
    std::list<GlucoseReading> deleteOldReadings(std::list<GlucoseReading> readings, unsigned long long epochToCompare);

  public:
    static NightscoutManager_ &getInstance();
    void setup();
    void tick();
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();
};

extern NightscoutManager_ &NightscoutManager;

#endif