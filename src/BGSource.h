#ifndef BGSOURCE_H
#define BGSOURCE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <list>
#include <utility>

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
    // A reference: with LibreLinkUp's one-minute readings the list holds about 180 entries, and
    // each copy allocated and freed one small block per entry, which fragmented the heap.
    virtual const std::list<GlucoseReading>& getGlucoseData() const;
    virtual String getStatus() const { return status; }

protected:
    HTTPClient* client;
    WiFiClientSecure* wifiSecureClient;
    unsigned long long lastCallAttemptEpoch = 0;
    bool firstConnectionSuccess = false;
    std::list<GlucoseReading> glucoseReadings;
    void deleteOldReadings(std::list<GlucoseReading>& readings, unsigned long long epochToCompare);
    // Gets the readings moved in and returns them with the new ones added, so the list is never
    // copied.
    virtual std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) = 0;
    BG_TREND parseDirection(String directionInput);
    void handleFailedAttempt();
    // With DEBUG_MEMORY, logs the WiFi signal and free memory around each request, so a log tells
    // a weak signal apart from memory too fragmented for the TLS handshake.
    void logNetworkAndMemoryStats(const char* stage, const char* request) const;
    String status = "not_initialized";
};

#endif  // BGSOURCE_H
