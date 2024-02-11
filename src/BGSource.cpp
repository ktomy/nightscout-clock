#include "BGSource.h"
#include "ServerManager.h"

unsigned long lastCallAttemptMills = 0;

void BGSource::setup() {
    client = new HTTPClient();
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
}

void BGSource::tick() {
    auto currentTime = ServerManager.getUtcEpoch();
    if (lastCallAttemptMills == 0 || currentTime > lastCallAttemptMills + 60 * 1000UL) {
        // delete readings older than now - 3 hours
        glucoseReadings = deleteOldReadings(glucoseReadings, time(NULL) - BG_BACKFILL_SECONDS);

        if (!firstConnectionSuccess) {
            DisplayManager.clearMatrix();
            DisplayManager.printText(0, 6, "To API", TEXT_ALIGNMENT::CENTER, 0);
        }

        glucoseReadings = updateReadings(glucoseReadings);

        lastCallAttemptMills = currentTime;
    }
}

std::list<GlucoseReading> BGSource::deleteOldReadings(std::list<GlucoseReading> readings, unsigned long long epochToCompare) {
    auto it = readings.begin();
    while (it != readings.end()) {
        if (it->epoch < epochToCompare) {
            it = readings.erase(it);
        } else {
            ++it;
        }
    }

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