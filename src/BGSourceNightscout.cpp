#include "BGSourceNightscout.h"
#include "ServerManager.h"
#include "SettingsManager.h"
#include "globals.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LCBUrl.h>
#include <StreamUtils.h>
#include <WiFiClientSecure.h>
#include <list>

unsigned long lastCallAttemptMills = 0;

void BGSourceNightscout::setup() {
    client = new HTTPClient();
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
    wifiClient = new WiFiClient();
}

void BGSourceNightscout::tick() {
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

            glucoseReadings = updateReadings(SettingsManager.settings.nightscout_url, SettingsManager.settings.nightscout_api_key,
                                             glucoseReadings);

            lastCallAttemptMills = currentTime;
        }
    }
}

BG_TREND parseDirection(String directionInput) {
    auto direction = directionInput;
    direction.toLowerCase();
    BG_TREND trend = NONE;
    if (direction == "doubleup") {
        trend = DOUBLE_UP;
    } else if (direction == "singleup") {
        trend = SINGLE_UP;
    } else if (direction == "fortyfiveup") {
        trend = FORTY_FIVE_UP;
    } else if (direction == "flat") {
        trend = FLAT;
    } else if (direction == "fortyfivedown") {
        trend = FORTY_FIVE_DOWN;
    } else if (direction == "singledown") {
        trend = SINGLE_DOWN;
    } else if (direction == "doubledown") {
        trend = DOUBLE_DOWN;
    } else if (direction == "not_computable") {
        trend = NOT_COMPUTABLE;
    } else if (direction == "rate_out_of_range") {
        trend = RATE_OUT_OF_RANGE;
    }

    return trend;
}

std::list<GlucoseReading> BGSourceNightscout::deleteOldReadings(std::list<GlucoseReading> readings,
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

std::list<GlucoseReading> BGSourceNightscout::updateReadings(String baseUrl, String apiKey,
                                                             std::list<GlucoseReading> existingReadings) {
    // set last epoch to now - 3 hours (we don't want to get too many readings)
    unsigned long long lastReadingEpoch = time(NULL) - 3 * 60 * 60;
    // get last epoch from existing readings if it is newer than default one
    if (existingReadings.size() > 0 && existingReadings.back().epoch > lastReadingEpoch) {
        lastReadingEpoch = existingReadings.back().epoch;
    }
    DEBUG_PRINTLN("Updating NS values since epoch: " + String(lastReadingEpoch) + " (-" +
                  String((time(NULL) - lastReadingEpoch) / 60) + "m)");

    // retrieve new readings until there are no new readings or until we reach
    // the point of now - 5 minutes (as we don't want readings from the future)
    do {
        // retrieve readings since lastReadingEpoch until lastReadingEpoch + 1 hour
        unsigned long long readToEpoch = lastReadingEpoch + 60 * 60;
        if (readToEpoch > time(NULL)) {
            readToEpoch = time(NULL);
        }

        std::list<GlucoseReading> retrievedReadings = retrieveReadings(baseUrl, apiKey, lastReadingEpoch, readToEpoch, 30);

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Retrieved readings: " + String(retrievedReadings.size()) +
                      ", last reading epoch: " + String(retrievedReadings.back().epoch) + " (-" +
                      String((time(NULL) - retrievedReadings.back().epoch) / 60) + "m)" +
                      " Difference between first reading and last reading in minutes: " +
                      String((retrievedReadings.back().epoch - retrievedReadings.front().epoch) / 60));
#endif

        // remove readings from retrievedReadings which are already present in existingReadings
        // because some servers (Gluroo) don't process from-to in an expected way
        retrievedReadings.remove_if([&existingReadings](const GlucoseReading &reading) {
            return std::find_if(existingReadings.begin(), existingReadings.end(),
                                [&reading](const GlucoseReading &existingReading) {
                                    return existingReading.epoch == reading.epoch;
                                }) != existingReadings.end();
        });

        if (retrievedReadings.size() == 0) {
            DEBUG_PRINTLN("No new readings");
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

std::list<GlucoseReading> BGSourceNightscout::retrieveReadings(String baseUrl, String apiKey,
                                                               unsigned long long readingSinceEpoch,
                                                               unsigned long long readingToEpoch, int numberOfvalues) {

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Getting NS values. Reading since epoch: " + String(readingSinceEpoch) + " (-" +
                  String((time(NULL) - readingSinceEpoch) / 60) + "m)" + ", number of values: " + String(numberOfvalues) +
                  ", reading to epoch: " + String(readingToEpoch) + " (-" + String((time(NULL) - readingToEpoch) / 60) + "m)");
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

        String responseContent = client->getString();

        DeserializationError error = deserializeJson(doc, responseContent);

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Response: " + responseContent);
#endif

        if (error) {
            DEBUG_PRINTF("Error deserializing NS response: %s\nFailed on string: %s\n", error.c_str(), responseContent.c_str());
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

bool BGSourceNightscout::hasNewData(unsigned long long epochToCompare) const {
    auto lastReadingEpoch = glucoseReadings.size() > 0 ? glucoseReadings.back().epoch : 0;
    return lastReadingEpoch > epochToCompare;
}

std::list<GlucoseReading> BGSourceNightscout::getGlucoseData() const { return glucoseReadings; }