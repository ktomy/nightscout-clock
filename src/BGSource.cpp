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
        }

        glucoseReadings = updateReadings(glucoseReadings);

        auto lastReading =
            glucoseReadings.size() > 0 ? glucoseReadings.back() : GlucoseReading{0, BG_TREND::NONE, 0};
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