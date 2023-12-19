#include "BGSource.h"

unsigned long lastCallAttemptMills = 0;

void BGSource::setup() {
    client = new HTTPClient();
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
}

void BGSource::tick() {
    auto currentTime = millis();
    if (lastCallAttemptMills == 0 || currentTime > lastCallAttemptMills + 60 * 1000UL) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            // delete readings older than now - 3 hours
            glucoseReadings = deleteOldReadings(glucoseReadings, time(NULL) - 3 * 60 * 60);

            if (!firstConnectionSuccess) {
                DisplayManager.clearMatrix();
                DisplayManager.printText(0, 6, "To API", TEXT_ALIGNMENT::CENTER, 0);
            }

            glucoseReadings = updateReadings(glucoseReadings);

            lastCallAttemptMills = currentTime;
        }
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

bool BGSource::hasNewData(unsigned long long epochToCompare) const {
    auto lastReadingEpoch = glucoseReadings.size() > 0 ? glucoseReadings.back().epoch : 0;
    return lastReadingEpoch > epochToCompare;
}

std::list<GlucoseReading> BGSource::getGlucoseData() const { return glucoseReadings; }