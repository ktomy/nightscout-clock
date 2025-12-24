#include "BGSourceNightscout.h"

#include <ArduinoJson.h>
#include <Hashing/Hash.h>
#include <StreamUtils.h>

#include "SettingsManager.h"
#include "globals.h"

std::list<GlucoseReading> BGSourceNightscout::updateReadings(
    std::list<GlucoseReading> existingReadings) {
    auto baseUrl = SettingsManager.settings.nightscout_url;
    auto apiKey = SettingsManager.settings.nightscout_api_key;

    return updateReadings(baseUrl, apiKey, existingReadings);
}

std::list<GlucoseReading> BGSourceNightscout::updateReadings(
    String baseUrl, String apiKey, std::list<GlucoseReading> existingReadings) {
    unsigned long long currentEpoch = ServerManager.getUtcEpoch();
    // set last epoch to now - 3 hours (we don't want to get too many readings)
    unsigned long long lastReadingEpoch = currentEpoch - BG_BACKFILL_SECONDS;
    // get last epoch from existing readings if it is newer than default one
    if (existingReadings.size() > 0 && existingReadings.back().epoch > lastReadingEpoch) {
        lastReadingEpoch = existingReadings.back().epoch;
    }
    DEBUG_PRINTLN(
        "Updating NS values since epoch: " + String(lastReadingEpoch) + " (-" +
        String((currentEpoch - lastReadingEpoch) / 60) + "m)");

    // retrieve new readings until there are no new readings or until we reach
    // the point of now - 5 minutes (as we don't want readings from the future)
    do {
        // retrieve readings since lastReadingEpoch until lastReadingEpoch + 1 hour
        unsigned long long readToEpoch = lastReadingEpoch + 60 * 60;
        if (readToEpoch > currentEpoch) {
            readToEpoch = currentEpoch;
        }

        // retrieve readings starting from the last reading plus one second (to not include the existing
        // reading)
        std::list<GlucoseReading> retrievedReadings =
            retrieveReadings(baseUrl, apiKey, lastReadingEpoch + 1, readToEpoch, 30);

        // if we didn't get any readings and the last reading is older than now, try to get the last
        // readings this is the case when there are gaps in readings for more than one hour for e.g.
        // sensor change
        if (retrievedReadings.size() == 0 && readToEpoch < currentEpoch) {
            DEBUG_PRINTLN("Second try to get readings");
            retrievedReadings =
                retrieveReadings(baseUrl, apiKey, lastReadingEpoch + 1, currentEpoch, 30);
        }

#ifdef DEBUG_BG_SOURCE
        if (retrievedReadings.size() == 0) {
            DEBUG_PRINTLN("Retrieved no readings");
        } else {
            DEBUG_PRINTLN(
                "Retrieved readings: " + String(retrievedReadings.size()) +
                ", last reading epoch: " + String(retrievedReadings.back().epoch) + " (-" +
                String((currentEpoch - retrievedReadings.back().epoch) / 60) + "m)" +
                " Difference between first reading and last reading in minutes: " +
                String((retrievedReadings.back().epoch - retrievedReadings.front().epoch) / 60));
        }
#endif

        // remove readings from retrievedReadings which are already present in existingReadings
        // because some servers (Gluroo) don't process from-to in an expected way
        retrievedReadings.remove_if([&existingReadings](const GlucoseReading& reading) {
            return std::find_if(
                       existingReadings.begin(), existingReadings.end(),
                       [&reading](const GlucoseReading& existingReading) {
                           return existingReading.epoch == reading.epoch;
                       }) != existingReadings.end();
        });

        if (retrievedReadings.size() == 0) {
            DEBUG_PRINTLN("No new readings");
            break;
        }
        // add retrieved reading to existing readings
        existingReadings.insert(
            existingReadings.end(), retrievedReadings.begin(), retrievedReadings.end());
        lastReadingEpoch = existingReadings.back().epoch;

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN(
            "Existing readings: " + String(existingReadings.size()) +
            ", last reading epoch: " + String(lastReadingEpoch) +
            " Difference to now is: " + String((currentEpoch - lastReadingEpoch) / 60) + " minutes");
#endif

    } while (lastReadingEpoch < currentEpoch - 5 * 60);
    return existingReadings;
}

std::list<GlucoseReading> BGSourceNightscout::retrieveReadings(
    String baseUrl, String apiKey, unsigned long long readingSinceEpoch,
    unsigned long long readingToEpoch, int numberOfvalues) {
    std::list<GlucoseReading> lastReadings;

    LCBUrl url = prepareUrl(baseUrl, readingSinceEpoch, readingToEpoch, numberOfvalues);

    bool ssl = url.getScheme() == "https";

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(ssl ? "SSL Active" : "No SSL")
#endif

    auto responseCode = initiateCall(url, ssl, apiKey);
    String responseContent = client->getString();
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Response: " + responseContent);
#endif

    if (responseCode == HTTP_CODE_OK) {
        JsonDocument doc;

        JsonDocument filter;
        filter[0]["sgv"] = true;
        filter[0]["date"] = true;
        filter[0]["trend"] = true;
        filter[0]["direction"] = true;

        DeserializationError error =
            deserializeJson(doc, responseContent, DeserializationOption::Filter(filter));

        if (error) {
            DEBUG_PRINTF(
                "Error deserializing NS response: %s\nFailed on string: %s\n", error.c_str(),
                responseContent.c_str());
            if (!firstConnectionSuccess) {
                status = "deserialization_error";
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
                if (v["trend"].is<int>()) {
                    reading.trend = (BG_TREND)(v["trend"].as<int>());
                } else if (v["direction"].is<String>()) {
                    reading.trend = parseDirection(v["direction"].as<String>());
                } else {
                    reading.trend = BG_TREND::NONE;
                }

                lastReadings.push_front(reading);
            }

            firstConnectionSuccess = true;
            status = "connected";

            // Sort readings by epoch (no idea if they come sorted from the API)
            lastReadings.sort([](const GlucoseReading& a, const GlucoseReading& b) -> bool {
                return a.epoch < b.epoch;
            });

            if (lastReadings.size() == 0) {
                DEBUG_PRINTLN("No readings received");
            } else {
                String debugLog = "Received readings: ";
                for (auto& reading : lastReadings) {
                    debugLog += " " + String(reading.sgv) + " -" + String(reading.getSecondsAgo() / 60) +
                                "m " + toString(reading.trend) + ", ";
                }

                debugLog += "\n";
                DEBUG_PRINTLN(debugLog);
            }
        }
        ServerManager.failedAttempts = 0;  // Reset failed attempts counter
    } else {
        DEBUG_PRINTF("Error getting readings %d\n", responseCode);
        if (!firstConnectionSuccess) {
            status = "connection_error";
            DisplayManager.showFatalError(String("Error connecting to Nightscout: ") + responseCode);
        }
        if (responseCode == -1) {
            handleFailedAttempt();
        }
    }

    client->end();
    return lastReadings;
}

LCBUrl BGSourceNightscout::prepareUrl(
    String baseUrl, unsigned long long readingSinceEpoch, unsigned long long readingToEpoch,
    int numberOfvalues) {
    unsigned long long currentEpoch = ServerManager.getUtcEpoch();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(
        "Getting NS values. Reading since epoch: " + String(readingSinceEpoch) + " (-" +
        String((currentEpoch - readingSinceEpoch) / 60) + "m)" + ", number of values: " +
        String(numberOfvalues) + ", reading to epoch: " + String(readingToEpoch) + " (-" +
        String((currentEpoch - readingToEpoch) / 60) + "m)");
#endif

    if (baseUrl == "") {
        status = "not_configured";
        DisplayManager.showFatalError(
            "Nightscout clock is not configured, please go to http://" + ServerManager.myIP.toString() +
            "/ and configure the device");
    }

    LCBUrl url;

    String sinceEpoch = String(readingSinceEpoch * 1000);
    String toEpoch = String(readingToEpoch * 1000);

    String urlString = String(
        baseUrl + "api/v1/entries?find[date][$gt]=" + sinceEpoch + "&find[date][$lte]=" + toEpoch +
        "&count=" + numberOfvalues);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("URL: " + urlString)
#endif

    auto urlIsOk = url.setUrl(urlString);
    if (!urlIsOk) {
        status = "invalid_url";
        DisplayManager.showFatalError("Invalid Nightscout URL: " + urlString);
    }
    return url;
}

int BGSourceNightscout::initiateCall(LCBUrl url, bool ssl, String apiKey) {
    if (ssl) {
        client->begin(
            *wifiSecureClient, url.getHost(), url.getPort(),
            String("/") + url.getPath() + url.getAfterPath(), true);
    } else {
        client->begin(url.getHost(), url.getPort(), String("/") + url.getPath() + url.getAfterPath());
    }

    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    client->addHeader("Accept", "application/json");

    if (apiKey != "") {
        auto hashedApiKey = sha1(apiKey);
        client->addHeader("api-secret", hashedApiKey);
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("API Key: " + apiKey);
        DEBUG_PRINTLN("Hashed API Key: " + hashedApiKey);
#endif
    }

    auto responseCode = client->GET();
    return responseCode;
}
