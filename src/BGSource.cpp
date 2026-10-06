#include "BGSource.h"

#include "ServerManager.h"

unsigned long lastCallAttemptMills = 0;

void BGSource::setup() {
    client = new HTTPClient();
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
    status = "initialized";
}

void BGSource::handleFailedAttempt() {
    ServerManager.failedAttempts++;
    if (ServerManager.failedAttempts >= 10) {
        ServerManager.reconnectWifi();
    }
}

void BGSource::tick() {
    unsigned long long currentTime = ServerManager.getUtcEpoch();
    if (lastCallAttemptEpoch == 0 || currentTime > lastCallAttemptEpoch + 60) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF(
            "BGSource::tick: Collecting data as > 60 seconds since last reading. Current time: %llu, "
            "last call attempt "
            "time: %llu (delta: %llu)",
            currentTime, lastCallAttemptEpoch, currentTime - lastCallAttemptEpoch);
#endif

        // delete readings older than now - 3 hours
        glucoseReadings = deleteOldReadings(glucoseReadings, currentTime - BG_BACKFILL_SECONDS);

        if (!firstConnectionSuccess) {
            DisplayManager.clearMatrix();
            DisplayManager.printText(0, 6, "To API", TEXT_ALIGNMENT::CENTER, 0);
            DisplayManager.update();
        }

        glucoseReadings = updateReadings(glucoseReadings);

        if (!lastFetchSucceeded) {
            // The fetch failed: escalate through a quick backoff ladder (1s, 5s,
            // 10s, 15s), then back off smartly. Past 4 failures, if the last
            // reading is under 15 minutes old the next data point should land on
            // the 5-minute CGM cadence, so wait for it; at 15+ minutes stale the
            // G7 retries every minute, so poll every minute.
            consecutiveFetchFailures++;
            int attempt = consecutiveFetchFailures;
            unsigned long retryDelaySec = 60;
            bool waitForNextPoint = false;
            if (consecutiveFetchFailures <= 4) {
                const unsigned long ladder[] = {1, 5, 10, 15};
                retryDelaySec = ladder[consecutiveFetchFailures - 1];
            } else {
                // Ladder exhausted: take a smart wait, then reset so the next
                // cycle starts with a fresh 1s/5s/10s/15s ladder.
                consecutiveFetchFailures = 0;
                auto lastReading = glucoseReadings.size() > 0 ? glucoseReadings.back()
                                                              : GlucoseReading{0, BG_TREND::NONE, 0};
                unsigned long long stalenessSec =
                    lastReading.epoch > 0 ? (currentTime - lastReading.epoch) : 0;
                if (lastReading.epoch > 0 && stalenessSec < 15 * 60) {
                    unsigned long long nextExpected = lastReading.epoch + 300 + 15;
                    if (nextExpected > currentTime + 60) {
                        // Trigger (currentTime > lastCallAttemptEpoch + 60) at nextExpected.
                        lastCallAttemptEpoch = nextExpected - 60;
                        waitForNextPoint = true;
                    }
                }
                // else: 15+ minutes stale (or no readings): retry every 60s.
            }
            if (!waitForNextPoint) {
                lastCallAttemptEpoch = currentTime + retryDelaySec - 60;
            }
            DEBUG_PRINTF("BGSource::tick: fetch failed (attempt %d), next try in %lus%s",
                         attempt, retryDelaySec,
                         waitForNextPoint ? " (waiting for next 5-min data point)" : "");
        } else {
            consecutiveFetchFailures = 0;
            auto lastReading =
                glucoseReadings.size() > 0 ? glucoseReadings.back()
                                             : GlucoseReading{0, BG_TREND::NONE, 0};
            if (lastReading.epoch > currentTime - 60 && lastReading.epoch < currentTime) {
#ifdef DEBUG_BG_SOURCE
                DEBUG_PRINTF(
                    "BGSource::tick: Adjusting lastCallAttemptEpoch to %llu from %llu (delte %llu)",
                    lastReading.epoch + 5, currentTime, currentTime - (lastReading.epoch + 5))
#endif
                // 5 seconds to save the value
                lastCallAttemptEpoch = lastReading.epoch + 5;
            } else {
#ifdef DEBUG_BG_SOURCE
                DEBUG_PRINTF(
                    "BGSource::tick: Not adjusting lastCallAttemptEpoch to last reading %llu from %llu "
                    "(delta: %llu)",
                    lastReading.epoch, currentTime, currentTime - (lastReading.epoch + 5))
#endif
                lastCallAttemptEpoch = currentTime;
            }
        }

    } else {
        // #ifdef DEBUG_BG_SOURCE

        //         // print debug message containing current time and last call attempt time
        //         DEBUG_PRINTF("BGSource::tick: Not collecting data as < 60 seconds since last reading.
        //         Current time: %llu, last call "
        //                      "attempt time: %llu",
        //                      currentTime, lastCallAttemptEpoch);

        // #endif
    }
}

std::list<GlucoseReading> BGSource::deleteOldReadings(
    std::list<GlucoseReading> readings, unsigned long long epochToCompare) {
    auto it = readings.begin();
    auto deletedCount = 0;
    while (it != readings.end()) {
        if (it->epoch < epochToCompare) {
            it = readings.erase(it);
            ++deletedCount;
        } else {
            ++it;
        }
    }

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Deleted %d old readings", deletedCount);
#endif

    return readings;
}

bool BGSource::hasNewData(unsigned long long epochToCompare) {
    auto lastReadingEpoch = glucoseReadings.size() > 0 ? glucoseReadings.back().epoch : 0;
    return lastReadingEpoch > epochToCompare;
}

std::list<GlucoseReading> BGSource::getGlucoseData() const { return glucoseReadings; }

BG_TREND BGSource::parseDirection(String directionInput) {
    auto direction = directionInput;
    direction.toLowerCase();
    BG_TREND trend = BG_TREND::NONE;
    if (direction == "doubleup") {
        trend = BG_TREND::DOUBLE_UP;
    } else if (direction == "singleup") {
        trend = BG_TREND::SINGLE_UP;
    } else if (direction == "fortyfiveup") {
        trend = BG_TREND::FORTY_FIVE_UP;
    } else if (direction == "flat") {
        trend = BG_TREND::FLAT;
    } else if (direction == "fortyfivedown") {
        trend = BG_TREND::FORTY_FIVE_DOWN;
    } else if (direction == "singledown") {
        trend = BG_TREND::SINGLE_DOWN;
    } else if (direction == "doubledown") {
        trend = BG_TREND::DOUBLE_DOWN;
    } else if (direction == "not_computable") {
        trend = BG_TREND::NOT_COMPUTABLE;
    } else if (direction == "rate_out_of_range") {
        trend = BG_TREND::RATE_OUT_OF_RANGE;
    }

    return trend;
}
