#include "BGSourceDexcom.h"

#include <ArduinoJson.h>
#include <LCBUrl.h>
#include <StreamUtils.h>

#include "SettingsManager.h"

std::list<GlucoseReading> BGSourceDexcom::updateReadings(std::list<GlucoseReading> existingReadings) {
    auto dexcomServer = SettingsManager.settings.dexcom_server;
    auto dexcomUsername = SettingsManager.settings.dexcom_username;
    auto dexcomPassword = SettingsManager.settings.dexcom_password;

    return updateReadings(dexcomServer, dexcomUsername, dexcomPassword, existingReadings);
}

std::list<GlucoseReading> BGSourceDexcom::updateReadings(
    DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword,
    std::list<GlucoseReading> existingReadings) {
    // set last epoch to now - 3 hours (we don't want to get too many readings)
    unsigned long long lastReadingEpoch = time(NULL) - BG_BACKFILL_SECONDS;
    // get last epoch from existing readings if it is newer than default one
    if (existingReadings.size() > 0 && existingReadings.back().epoch > lastReadingEpoch) {
        lastReadingEpoch = existingReadings.back().epoch;
    }

    int forMinutes = (time(NULL) - lastReadingEpoch) / 60;
    int readingsCount = forMinutes / 5 + 2;  // 2 is just for safety :)
    DEBUG_PRINTLN(
        "Updating Dexcom values since epoch: " + String(lastReadingEpoch) + " (-" + String(forMinutes) +
        "m), readings count: " + String(readingsCount));

    auto retrievedReadings =
        retrieveReadings(dexcomServer, dexcomUsername, dexcomPassword, forMinutes, readingsCount);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(
        "Retrieved readings: " + String(retrievedReadings.size()) +
        ", last reading epoch: " + String(retrievedReadings.back().epoch) + " (-" +
        String((time(NULL) - retrievedReadings.back().epoch) / 60) + "m)" +
        " Difference between first reading and last reading in minutes: " +
        String((retrievedReadings.back().epoch - retrievedReadings.front().epoch) / 60));
#endif

    // remove readings from retrievedReadings which are already present in existingReadings
    // to eliminate duplicate readins
    retrievedReadings.remove_if([&existingReadings](const GlucoseReading& reading) {
        return std::find_if(
                   existingReadings.begin(), existingReadings.end(),
                   [&reading](const GlucoseReading& existingReading) {
                       return existingReading.epoch == reading.epoch;
                   }) != existingReadings.end();
    });

    if (retrievedReadings.size() == 0) {
        DEBUG_PRINTLN("No new readings");
        return existingReadings;
    }

    // add readings to existing readings
    existingReadings.insert(existingReadings.end(), retrievedReadings.begin(), retrievedReadings.end());

    // set lastReadingEpoch to the last reading epoch
    lastReadingEpoch = retrievedReadings.back().epoch;

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(
        "Existing readings: " + String(existingReadings.size()) +
        ", last reading epoch: " + String(lastReadingEpoch) +
        " Difference to now is: " + String((time(NULL) - lastReadingEpoch) / 60) + " minutes");
#endif

    return existingReadings;
}

std::list<GlucoseReading> BGSourceDexcom::retrieveReadings(
    DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword, int forMinutes,
    int readingsCount) {
    std::list<GlucoseReading> readings;
    if (accountId.length() == 0) {
        accountId = getAccountId(dexcomServer, dexcomUsername, dexcomPassword);
    }
    if (accountId.length() == 0) {
        DEBUG_PRINTLN("Cannot retrieve Dexcom account Id");
        return readings;
    }
    if (sessionId.length() == 0) {
        sessionId = getSessionId(dexcomServer, accountId, dexcomPassword);
    }
    if (sessionId.length() == 0) {
        DEBUG_PRINTLN("Cannot retrieve Dexcom session Id");
        return readings;
    }

    String readingsUrl = "";
    switch (dexcomServer) {
        case DEXCOM_SERVER::US:
            readingsUrl += DEXCOM_US_SERVER;
            break;
        case DEXCOM_SERVER::NON_US:
            readingsUrl += DEXCOM_NON_US_SERVER;
            break;
        case DEXCOM_SERVER::JAPAN:
            readingsUrl += DEXCOM_JAPAN_SERVER;
            break;
        default:
            DisplayManager.showFatalError("Invalid Dexcom server " + String((uint8_t)dexcomServer));
            return readings;
    }

    readingsUrl += DEXCOM_GET_GLUCOSE_READINGS_PATH;
    readingsUrl += "?sessionId=" + sessionId;
    readingsUrl += "&minutes=" + String(forMinutes);
    readingsUrl += "&maxCount=" + String(readingsCount);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Getting Dexcom readings, URL: " + readingsUrl);
#endif

    LCBUrl url;
    url.setUrl(readingsUrl);

    client->begin(
        *wifiSecureClient, url.getHost(), url.getPort(),
        String("/") + url.getPath() + url.getAfterPath(), true);
    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    client->addHeader("Accept", "application/json");
    client->addHeader("User-Agent", "Nightscout-clock");
    auto responseCode = client->GET();
    if (responseCode == HTTP_CODE_OK) {
        JsonDocument doc;

        String responseContent = client->getString();

        DeserializationError error = deserializeJson(doc, responseContent);

#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Response: " + responseContent);
#endif

        if (error) {
            DEBUG_PRINTF(
                "Error deserializing dexcom response: %s\nFailed on string: %s\n", error.c_str(),
                responseContent.c_str());
            if (!firstConnectionSuccess) {
                DisplayManager.showFatalError(String("Invalid Dexcom Share response: ") + error.c_str());
            }

            doc.clear();
            return readings;
        }

        if (doc.is<JsonArray>()) {
            JsonArray jsonArray = doc.as<JsonArray>();
            for (JsonVariant v : jsonArray) {
                GlucoseReading reading;
                reading.sgv = v["Value"].as<int>();
                auto epochString = v["ST"].as<String>();
                // Trim Date(1703182152000) to 1703182152
                epochString = epochString.substring(5, epochString.length() - 4);
                // convert to unsigned long long
                reading.epoch = strtoull(epochString.c_str(), NULL, 10);
                if (v["Trend"].is<String>()) {
                    reading.trend = parseDirection(v["Trend"].as<String>());
                } else {
                    reading.trend = BG_TREND::NONE;
                }

                readings.push_front(reading);
            }

            firstConnectionSuccess = true;

            // Sort readings by epoch (no idea if they come sorted from the API)
            readings.sort([](const GlucoseReading& a, const GlucoseReading& b) -> bool {
                return a.epoch < b.epoch;
            });

            String debugLog = "Received readings: ";
            for (auto& reading : readings) {
                debugLog += " " + String(reading.sgv) + " -" + String(reading.getSecondsAgo() / 60) +
                            "m " + toString(reading.trend) + ", ";
            }

            debugLog += "\n";
            DEBUG_PRINTLN(debugLog);
        } else {
            DEBUG_PRINTLN("Dexcom response is not an array");
        }

        doc.clear();

        ServerManager.failedAttempts = 0;  // Reset failed attempts counter
    } else {
        DEBUG_PRINTF("Error getting readings %d\n", responseCode);
        if (responseCode == HTTP_CODE_INTERNAL_SERVER_ERROR) {
            auto errorResponseString = client->getString();
            if (errorResponseString.indexOf("Session") != -1) {
                sessionId = "";
                return retrieveReadings(
                    dexcomServer, dexcomUsername, dexcomPassword, forMinutes, readingsCount);
            }
        }
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError(String("Error connecting to Dexcom server: ") + responseCode);
        }
        if (responseCode == -1) {
            handleFailedAttempt();
        }
        return readings;
    }

    client->end();
    return readings;
}

String BGSourceDexcom::getAccountId(
    DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword) {
    String accountId = "";
    String getAccountIdUrl = "";
    switch (dexcomServer) {
        case DEXCOM_SERVER::US:
            getAccountIdUrl += DEXCOM_US_SERVER;
            break;
        case DEXCOM_SERVER::NON_US:
            getAccountIdUrl += DEXCOM_NON_US_SERVER;
            break;
        case DEXCOM_SERVER::JAPAN:
            getAccountIdUrl += DEXCOM_JAPAN_SERVER;
            break;
        default:
            DisplayManager.showFatalError("Invalid Dexcom server " + String((uint8_t)dexcomServer));
            return "";
    }

    getAccountIdUrl += DEXCOM_GET_ACCOUNT_ID_PATH;

    // create JSON body having username, password and applicationId
    JsonDocument doc;
    doc["accountName"] = dexcomUsername;
    doc["password"] = dexcomPassword;
    doc["applicationId"] =
        (dexcomServer == DEXCOM_SERVER::JAPAN) ? DEXCOM_APPLICATION_ID_JAPAN : DEXCOM_APPLICATION_ID;

    String body;
    serializeJson(doc, body);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Getting account Id, Url: " + getAccountIdUrl + ", body: " + body);
#endif

    // Do the POST call
    client->begin(*wifiSecureClient, getAccountIdUrl);
    client->addHeader("Content-Type", "application/json");
    client->addHeader("Accept", "application/json");
    client->addHeader("User-Agent", "Nightscout-clock");
    auto responseCode = client->POST(body);
    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Error getting account id %d\n", responseCode);
        if (responseCode == HTTP_CODE_INTERNAL_SERVER_ERROR) {
            auto errorResponseString = client->getString();
            if (errorResponseString.indexOf("AccountPasswordInvalid") != -1) {
                DisplayManager.showFatalError(
                    "Invalid Dexcom username/email or password. Go to http://" +
                    ServerManager.myIP.toString() + "/ to fix.");
            }
        }
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError(String("Error connecting to Dexcom server: ") + responseCode);
        }

        client->end();
        return "";
    }

    String response = client->getString();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Account Id call response: " + response);
#endif

    client->end();

    if (response.length() > 0) {
        String accountIdString = response.substring(1, response.length() - 1);
        return accountIdString;
    }

    return "";
}

String BGSourceDexcom::getSessionId(
    DEXCOM_SERVER dexcomServer, String accountId, String dexcomPassword) {
    String sessionId = "";
    String getSessionIdUrl = "";
    switch (dexcomServer) {
        case DEXCOM_SERVER::US:
            getSessionIdUrl += DEXCOM_US_SERVER;
            break;
        case DEXCOM_SERVER::NON_US:
            getSessionIdUrl += DEXCOM_NON_US_SERVER;
            break;
        case DEXCOM_SERVER::JAPAN:
            getSessionIdUrl += DEXCOM_JAPAN_SERVER;
            break;
        default:
            DisplayManager.showFatalError("Invalid Dexcom server " + String((uint8_t)dexcomServer));
            return "";
    }

    getSessionIdUrl += DEXCOM_GET_SESSION_ID_PATH;

    // create JSON body having accountId, password and applicationId
    JsonDocument doc;
    doc["accountId"] = accountId;
    doc["password"] = dexcomPassword;
    doc["applicationId"] =
        (dexcomServer == DEXCOM_SERVER::JAPAN) ? DEXCOM_APPLICATION_ID_JAPAN : DEXCOM_APPLICATION_ID;

    String body;
    serializeJson(doc, body);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Getting session Id, Url: " + getSessionIdUrl + ", body: " + body);
#endif

    // Do the POST call
    client->begin(*wifiSecureClient, getSessionIdUrl);
    client->addHeader("Content-Type", "application/json");
    client->addHeader("Accept", "application/json");
    client->addHeader("User-Agent", "Nightscout-clock");
    auto responseCode = client->POST(body);
    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Error getting session id %d\n", responseCode);
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError(String("Error connecting to Dexcom server: ") + responseCode);
        }

        client->end();
        return "";
    }

    String response = client->getString();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Session Id call response: " + response);
#endif

    client->end();

    if (response.length() > 0) {
        String sessionIdString = response.substring(1, response.length() - 1);
        return sessionIdString;
    }

    return "";
}
