#ifndef BGSOURCE_H
#define BGSOURCE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <list>

#include "DisplayManager.h"
#include "ServerManager.h"
#include "enums.h"
#include "globals.h"

#define BG_BACKFILL_SECONDS \
    3 * 60 * 60 + 1  // 3 hours one second (to overcome the "after last reading" read)

struct GlucoseReading {
public:
    int sgv;
    BG_TREND trend;
    unsigned long long epoch;

    int getSecondsAgo() const { return time(NULL) - epoch; }

    bool isEmpty() const { return sgv == 0 && trend == BG_TREND::NONE && epoch == 0; }

    String toString() const {
        return String(sgv) + "," + ::toString(trend) + "," + String(epoch) + "(" +
               String(getSecondsAgo()) + ")";
    }
};

class BGSource {
public:
    virtual void setup();
    virtual void tick();
    virtual bool hasNewData(unsigned long long epochToCompare);
    virtual std::list<GlucoseReading> getGlucoseData() const;

protected:
    HTTPClient* client;
    WiFiClientSecure* wifiSecureClient;
    unsigned long long lastCallAttemptEpoch = 0;
    bool firstConnectionSuccess = false;
    std::list<GlucoseReading> glucoseReadings;
    std::list<GlucoseReading> deleteOldReadings(
        std::list<GlucoseReading> readings, unsigned long long epochToCompare);
    virtual std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) = 0;
    BG_TREND parseDirection(String directionInput);
    void handleFailedAttempt();
};

#endif  // BGSOURCE_H
