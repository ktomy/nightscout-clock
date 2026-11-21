#ifndef BGSOURCE_H
#define BGSOURCE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <atomic>
#include <list>

#include "DisplayManager.h"
#include "GlucoseReadingBuffer.h"
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

// Three hours of one-minute readings are 181, plus room for readings that arrive a little early.
// The data source and the display each keep one, 3 KB each.
using GlucoseReadings = GlucoseReadingBuffer<GlucoseReading, 192>;

class AsyncWebServerRequest;

class BGSource {
public:
    virtual void setup();
    virtual void tick();
    // Stops the source for good before a firmware update: closes its connection and frees the HTTP
    // and TLS clients. A source that holds more memory frees it in an override. The object itself
    // stays, so the web handlers a source added keep a valid `this`; they answer 503 from then on.
    virtual void end();
    bool isStopped() const { return stopped; }
    virtual bool hasNewData(unsigned long long epochToCompare);
    virtual const GlucoseReadings& getGlucoseData() const;
    virtual String getStatus() const { return status; }

protected:
    HTTPClient* client = nullptr;
    WiFiClientSecure* wifiSecureClient = nullptr;
    // Read by the web server task, set by the main loop.
    std::atomic<bool> stopped{false};
    // For the web handlers a source adds: true, after answering 503, once the source is stopped.
    bool rejectIfStopped(AsyncWebServerRequest* request) const;
    unsigned long long lastCallAttemptEpoch = 0;
    bool firstConnectionSuccess = false;
    GlucoseReadings glucoseReadings;
    // Requests the readings newer than the newest in `readings` and adds them.
    virtual void updateReadings(GlucoseReadings& readings) = 0;
    BG_TREND parseDirection(String directionInput);
    void handleFailedAttempt();
    // With DEBUG_MEMORY, logs the WiFi signal and free memory around each request, so a log tells
    // a weak signal apart from memory too fragmented for the TLS handshake.
    void logNetworkAndMemoryStats(const char* stage, const char* request) const;
    String status = "not_initialized";
};

#endif  // BGSOURCE_H
