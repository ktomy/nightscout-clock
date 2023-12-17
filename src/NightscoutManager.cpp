#include "NightscoutManager.h"
#include "ServerManager.h"
#include "SettingsManager.h"
#include "globals.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LCBUrl.h>
#include <WiFiClientSecure.h>
#include <list>
// #include <StreamUtils.h>

// The getter for the instantiated singleton instance
NightscoutManager_ &NightscoutManager_::getInstance() {
    static NightscoutManager_ instance;
    return instance;
}

// Initialize the global shared instance
NightscoutManager_ &NightscoutManager = NightscoutManager.getInstance();

unsigned long lastCallAttemptMills = 0;

void NightscoutManager_::setup() {
    client = new HTTPClient();
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
    wifiClient = new WiFiClient();
}

void NightscoutManager_::tick() {
    auto currentTime = millis();
    if (lastCallAttemptMills == 0 || currentTime > lastCallAttemptMills + 60 * 1000UL) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            // delete readings older than now - 3 hours
            glucoseReadings = deleteOldReadings(glucoseReadings, time(NULL) - 3 * 60 * 60);

            if (!firstConnectionSuccess) {
                DisplayManager.clearMatrix();
                DisplayManager.printText(0, 6, "To API", CENTER, 0);
            }

            glucoseReadings = updateReadings(SettingsManager.settings.nsUrl, SettingsManager.settings.nsApiKey, glucoseReadings);

            lastCallAttemptMills = currentTime;
        }
    }
}

BG_TREND parseDirection(String directionInput) {
    auto direction = directionInput;
    direction.toLowerCase();
    BG_TREND trend = NONE;
    if (direction == "doubleup") {
        trend = DoubleUp;
    } else if (direction == "singleup") {
        trend = SingleUp;
    } else if (direction == "fortyfiveup") {
        trend = FortyFiveUp;
    } else if (direction == "flat") {
        trend = Flat;
    } else if (direction == "fortyfivedown") {
        trend = FortyFiveDown;
    } else if (direction == "singledown") {
        trend = SingleDown;
    } else if (direction == "doubledown") {
        trend = DoubleDown;
    } else if (direction == "not_computable") {
        trend = NOT_COMPUTABLE;
    } else if (direction == "rate_out_of_range") {
        trend = RATE_OUT_OF_RANGE;
    }

    return trend;
}

std::list<GlucoseReading> NightscoutManager_::deleteOldReadings(std::list<GlucoseReading> readings,
                                                                unsigned long long epochToCompare) {
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

std::list<GlucoseReading> NightscoutManager_::updateReadings(String baseUrl, String apiKey,
                                                             std::list<GlucoseReading> existingReadings) {
    // set last epoch to now - 3 hours (we don't want to get too many readings)
    unsigned long long lastReadingEpoch = time(NULL) - 3 * 60 * 60;
    // get last epoch from existing readings if it is newer than default one
    if (existingReadings.size() > 0 && existingReadings.back().epoch > lastReadingEpoch) {
        lastReadingEpoch = existingReadings.back().epoch;
    }
    DEBUG_PRINTLN("Updating NS values since epoch: " + String(lastReadingEpoch));

    // retrieve new readings until there are no new readings or until we reach
    // the point of now - 5 minutes (as we don't want readings from the future)
    do {
        // retrieve readings since lastReadingEpoch until lastReadingEpoch + 1 hour
        std::list<GlucoseReading> retrievedReadings =
            retrieveReadings(baseUrl, apiKey, lastReadingEpoch, lastReadingEpoch + 60 * 60, 60);

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Retrieved readings: " + String(retrievedReadings.size()) + ", last reading epoch: " +
                      String(retrievedReadings.back().epoch) + " Difference between first reading and last reading in minutes: " +
                      String((retrievedReadings.back().epoch - retrievedReadings.front().epoch) / 60));
#endif

        if (retrievedReadings.size() == 0) {
            DEBUG_PRINTLN("No more readings");
            break;
        }
        // add retrieved reading to existing readings
        existingReadings.insert(existingReadings.end(), retrievedReadings.begin(), retrievedReadings.end());
        lastReadingEpoch = existingReadings.back().epoch;

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Existing readings: " + String(existingReadings.size()) +
                      ", last reading epoch: " + String(lastReadingEpoch) +
                      " Difference to now is: " + String((time(NULL) - lastReadingEpoch) / 60) + " minutes");
#endif

    } while (lastReadingEpoch < time(NULL) - 5 * 60);
    return existingReadings;
}

std::list<GlucoseReading> NightscoutManager_::retrieveReadings(String baseUrl, String apiKey,
                                                               unsigned long long readingSinceEpoch,
                                                               unsigned long long readingToEpoch, int numberOfvalues) {

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Getting NS values. Reading since epoch: " + String(readingSinceEpoch) +
                  ", number of values: " + String(numberOfvalues) + ", reading to epoch: " + String(readingToEpoch));
#endif

    if (baseUrl == "") {
        DisplayManager.showFatalError("Nightscout clock is not configured, please go to http://" + ServerManager.myIP.toString() +
                                      "/ and configure the device");
    }

    LCBUrl url;

    String sinceEpoch = String(readingSinceEpoch * 1000);
    String toEpoch = String(readingToEpoch * 1000);

    String urlString = String(baseUrl + "api/v1/entries?find[date][$gt]=" + sinceEpoch + "&find[date][$lte]=" + toEpoch +
                              "&count=" + numberOfvalues);
    if (apiKey != "") {
        urlString += "&token=" + apiKey;
    }

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("URL: " + urlString)
#endif

    auto urlIsOk = url.setUrl(urlString);
    if (!urlIsOk) {
        DisplayManager.showFatalError("Invalid Nightscout URL: " + urlString);
    }

    bool ssl = url.getScheme() == "https";

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(ssl ? "SSL Active" : "No SSL")
#endif
    if (ssl) {
        client->begin(*wifiSecureClient, url.getHost(), url.getPort(), String("/") + url.getPath() + url.getAfterPath(), true);
    } else {
        client->begin(url.getHost(), url.getPort(), String("/") + url.getPath() + url.getAfterPath());
    }

    std::list<GlucoseReading> lastReadings;

    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    client->addHeader("Accept", "application/json");
    auto responseCode = client->GET();
    if (responseCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(0xFFFF);

        DeserializationError error = deserializeJson(doc, client->getStream());

        if (error) {
            DEBUG_PRINTF("Error deserializing NS response: %s\n", error.c_str());
            if (!firstConnectionSuccess) {
                DisplayManager.showFatalError(String("Invalid Nightscout response: ") + error.c_str());
            }

            return lastReadings;
        }

        if (doc.is<JsonArray>()) {
            JsonArray jsonArray = doc.as<JsonArray>();
            for (JsonVariant v : jsonArray) {
                GlucoseReading reading;
                reading.sgv = v["sgv"].as<int>();
                reading.epoch = v["date"].as<unsigned long long>() / 1000;
                // sensor.epoch = v["mills"].as<unsigned long>();
                if (v.containsKey("trend")) {
                    reading.trend = (BG_TREND)(v["trend"].as<int>());
                } else if (v.containsKey("direction")) {
                    reading.trend = parseDirection(v["direction"].as<String>());
                } else {
                    reading.trend = NONE;
                }

                lastReadings.push_front(reading);
            }

            firstConnectionSuccess = true;

            // Sort readings by epoch (no idea if they come sorted from the API)
            lastReadings.sort([](const GlucoseReading &a, const GlucoseReading &b) -> bool { return a.epoch < b.epoch; });

            String debugLog = "Received readings: ";
            for (auto &reading : lastReadings) {
                debugLog +=
                    " " + String(reading.sgv) + " " + String(reading.getSecondsAgo() / 60) + "m " + String(reading.trend) + ", ";
            }

            debugLog += "\n";
            DEBUG_PRINTLN(debugLog);
        }
    } else {
        DEBUG_PRINTF("Error getting readings %d\n", responseCode);
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError(String("Error connecting to Nightscout: ") + responseCode);
        }
    }

    client->end();
    return lastReadings;
}

bool NightscoutManager_::hasNewData(unsigned long long epochToCompare) {
    auto lastReadingEpoch = glucoseReadings.size() > 0 ? glucoseReadings.back().epoch : 0;
    return lastReadingEpoch > epochToCompare;
}
std::list<GlucoseReading> NightscoutManager_::getGlucoseData() { return glucoseReadings; }