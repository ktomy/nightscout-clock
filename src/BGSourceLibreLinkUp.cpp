#include <Arduino.h>
#include "BGSourceLibreLinkUp.h"

bool BGSourceLibreLinkUp::hasValidAuthentication() {

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Checking if authentication is valid, token: %s, expires: %llu, now: %llu\n", authTicket.token.c_str(),
                 authTicket.expires, time(NULL));
#endif

    if (authTicket.token == "") {
        return false;
    }
    if (authTicket.expires < time(NULL)) {
        return false;
    }

    return true;
}
void BGSourceLibreLinkUp::deleteAuthTicket() {
    authTicket.token = "";
    authTicket.expires = 0;
    authTicket.duration = 0;
    authTicket.accountId = "";
    authTicket.patientId = "";
}

AuthTicket BGSourceLibreLinkUp::login() {

    if (libreEndpoints.find(SettingsManager.settings.librelinkup_region) == libreEndpoints.end()) {
        DEBUG_PRINTF("Invalid region %s\n", SettingsManager.settings.librelinkup_region.c_str());
        DisplayManager.showFatalError("Invalid LibreLinkUp region, please check settings");
    }

    String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/auth/login";

    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000); // Set timeout to 15 seconds (15000 milliseconds)
    for (auto &header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    String body = "{\"email\":\"" + SettingsManager.settings.librelinkup_email + "\",\"password\":\"" +
                  SettingsManager.settings.librelinkup_password + "\"}";

    auto responseCode = client->POST(body);
    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Error logging in to LibreLinkUp %d\n", responseCode);
        DisplayManager.showFatalError(String("Error logging in to LibreLinkUp: ") + String(responseCode));
    }

    String response = client->getString();

    // #ifdef DEBUG_BG_SOURCE
    //     DEBUG_PRINTLN("Response: " + response);
    // #endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        DEBUG_PRINTF("Error deserializing LibreLinkUp login response: %s\nFailed on string: %s\n", error.c_str(),
                     response.c_str());
        DisplayManager.showFatalError(String("Invalid LibreLinkUp login response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF("Failed to login to LibreLinkUp, non-zero status %s\n", response.c_str());
        DisplayManager.showFatalError("Failed to login to LibreLinkUp, check credentials");
    }

    if (doc["data"]["redirect"].as<bool>()) {
        String region = doc["data"]["region"].as<String>();
        region.toUpperCase();
        SettingsManager.settings.librelinkup_region = region;
        SettingsManager.saveSettingsToFile();
        DEBUG_PRINTF("Redirecting to region %s\n", region.c_str());
        doc.clear();
        client->end();
        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_CYAN);
        DisplayManager.printText(0, 6, "Redirect...", TEXT_ALIGNMENT::CENTER, 0);
        delay(2000);
        ESP.restart();
    }

    String token = doc["data"]["authTicket"]["token"].as<String>();
    unsigned long long expires = doc["data"]["authTicket"]["expires"].as<unsigned long long>();
    unsigned long long duration = doc["data"]["authTicket"]["duration"].as<unsigned long long>();
    String accountId = doc["data"]["user"]["id"].as<String>();
    auto authTicket = AuthTicket(token, expires, duration, accountId);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Logged in to LibreLinkUp, token: %s, expires: %llu, duration: %llu, accountId %s\n", token.c_str(), expires,
                 duration, accountId.c_str());
#endif

    return authTicket;
}

unsigned long long BGSourceLibreLinkUp::libreFactoryTimestampToEpoch(String dateString) {
    setenv("TZ", "UTC0", 1);
    tzset();
    struct tm tm;
    strptime(dateString.c_str(), "%m/%d/%Y %I:%M:%S %p", &tm);
    time_t t = mktime(&tm);
    setenv("TZ", SettingsManager.settings.tz_libc_value.c_str(), 1);
    tzset();
    return t;
}

BG_TREND trendFromLibreTrend(int trend) {
    switch (trend) {
        case 5:
            return BG_TREND::SINGLE_UP;
        case 4:
            return BG_TREND::FORTY_FIVE_UP;
        case 3:
            return BG_TREND::FLAT;
        case 2:
            return BG_TREND::FORTY_FIVE_DOWN;
        case 1:
            return BG_TREND::SINGLE_DOWN;
        default:
            return BG_TREND::NONE;
    }
}

GlucoseReading BGSourceLibreLinkUp::getLibreLinkUpConnection() {

    String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/connections";

    client->begin(url);
    for (auto &header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    auto responseCode = client->GET();

    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Error getting connections from LibreLinkUp %d\n", responseCode);
        DisplayManager.showFatalError(String("Error getting connections from LibreLinkUp: ") + String(responseCode));
    }

    String response = client->getString();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Connection Response: " + response);
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        DEBUG_PRINTF("Error deserializing LibreLinkUp connections response: %s\nFailed on string: %s\n", error.c_str(),
                     response.c_str());
        DisplayManager.showFatalError(String("Invalid LibreLinkUp connections response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF("Failed to get connections from LibreLinkUp, non-zero status %s\n", response.c_str());
        DisplayManager.showFatalError("Failed to get connections from LibreLinkUp, please restart");
    }

    if (doc["data"].size() == 0) {
        DEBUG_PRINTLN("No LibreLinkUp connections found");
        DisplayManager.showFatalError("No LibreLinkUp connections found");
    }

    String patientId = doc["data"][0]["patientId"].as<String>();
    String firstName = doc["data"][0]["firstName"].as<String>();
    String lastName = doc["data"][0]["lastName"].as<String>();

    authTicket.patientId = patientId;

    GlucoseReading reading;
    reading.sgv = doc["data"][0]["glucoseMeasurement"]["ValueInMgPerDl"].as<int>();
    String dateString = doc["data"][0]["glucoseMeasurement"]["FactoryTimestamp"].as<String>();
    reading.epoch = libreFactoryTimestampToEpoch(dateString);
    reading.trend = trendFromLibreTrend(doc["data"][0]["glucoseMeasurement"]["TrendArrow"].as<int>());

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Got LibreLinkUp connection, patientId: %s, firstName: %s, lastName: %s, last reading: %s\n", patientId.c_str(),
                 firstName.c_str(), lastName.c_str(), reading.toString().c_str());
#endif

    return reading;
}

#define SHA256_DIGEST_SIZE 32

String BGSourceLibreLinkUp::encodeSHA256(String toEncode) {
    unsigned char hash[SHA256_DIGEST_SIZE];
    mbedtls_sha256((const unsigned char *)toEncode.c_str(), toEncode.length(), hash, 0);
    String encoded = "";
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        encoded += buf;
    }
    return encoded;
}

std::list<GlucoseReading> BGSourceLibreLinkUp::updateReadings(std::list<GlucoseReading> existingReadings) {

    if (!hasValidAuthentication()) {
        deleteAuthTicket();
        auto newAuthTicket = login();
        if (newAuthTicket.token == "") {
            DisplayManager.showFatalError("Failed to login to LibreLinkUp, check credentials");
        }

        authTicket = newAuthTicket;
    }

    unsigned long long lastReadingEpoch = existingReadings.size() > 0 ? existingReadings.back().epoch : 0;

    auto glucoseReadings = getReadings(lastReadingEpoch);

    // sort the readings by epoch
    glucoseReadings.sort([](const GlucoseReading &a, const GlucoseReading &b) { return a.epoch < b.epoch; });

    // remove readings from retrievedReadings which are already present in existingReadings
    // because some servers (Gluroo) don't process from-to in an expected way
    glucoseReadings.remove_if([&existingReadings](const GlucoseReading &reading) {
        return std::find_if(existingReadings.begin(), existingReadings.end(), [&reading](const GlucoseReading &existingReading) {
                   return existingReading.epoch == reading.epoch;
               }) != existingReadings.end();
    });

    if (glucoseReadings.size() == 0) {
        DEBUG_PRINTLN("No new readings");
        return existingReadings;
    }

    // add retrieved reading to existing readings
    existingReadings.insert(existingReadings.end(), glucoseReadings.begin(), glucoseReadings.end());

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Existing readings: " + String(existingReadings.size()) +
                  ", last reading epoch: " + String(existingReadings.back().epoch) +
                  " Difference to now is: " + String((time(NULL) - existingReadings.back().epoch) / 60) + " minutes");

    // print 5 last readings if they exist in format: "SGV - minutes ago, trend, "
    if (existingReadings.size() > 0) {
        String debugLog = "Received readings: ";
        int count = 0;
        for (auto it = existingReadings.rbegin(); it != existingReadings.rend() && count < 5; ++it, ++count) {
            debugLog += " " + String(it->sgv) + " -" + String(it->getSecondsAgo() / 60) + "m " + toString(it->trend) + ", ";
        }

        debugLog += "\n";
        DEBUG_PRINTLN(debugLog);
    }
#endif

    return existingReadings;
}

std::list<GlucoseReading> BGSourceLibreLinkUp::getReadings(unsigned long long lastReadingEpoch) {

    auto readingFromConnection = getLibreLinkUpConnection();

    if (lastCallAttemptEpoch >= readingFromConnection.epoch) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF("No readings needed, last reading epoch %llu is already in\n", lastCallAttemptEpoch);
#endif
        return std::list<GlucoseReading>();
    }

    if (lastReadingEpoch >= readingFromConnection.epoch - 60 * 5) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF("No historical data needed, last reading is %d seconds older then the current reading\n",
                     readingFromConnection.epoch - lastReadingEpoch);
#endif
        return std::list<GlucoseReading>{readingFromConnection};
    }

    String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/connections/" +
                 authTicket.patientId + "/graph";

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Getting glucose from LibreLinkUp, url: %s\n", url.c_str());
#endif

    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000); // Set timeout to 15 seconds (15000 milliseconds)
    for (auto &header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    auto responseCode = client->GET();
    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Error getting glucose from LibreLinkUp %d\n", responseCode);
        DisplayManager.showFatalError(String("Error getting connections from LibreLinkUp: ") + String(responseCode));
    }

    String response = client->getString();

    // #ifdef DEBUG_BG_SOURCE
    //     DEBUG_PRINTLN("Response: " + response);
    // #endif

    firstConnectionSuccess = true;

    JsonDocument doc;

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Free memory: %d\n", ESP.getFreeHeap());
#endif

    JsonDocument filter;
    filter["data"]["graphData"][0]["ValueInMgPerDl"] = true;
    filter["data"]["graphData"][0]["FactoryTimestamp"] = true;
    filter["status"] = true;

    DeserializationOption::Filter filterOption(filter);

    DeserializationError error = deserializeJson(doc, response, filterOption);
    // DeserializationError error = deserializeJson(doc, response);
    if (error) {
        DEBUG_PRINTF("Error deserializing LibreLinkUp glucose response: %s\nFailed on string: %s\n", error.c_str(),
                     response.c_str());
        DisplayManager.showFatalError(String("Invalid LibreLinkUp glucose response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF("Failed to get glucose from LibreLinkUp, non-zero status %s\n", response.c_str());
        DisplayManager.showFatalError("Failed to get glucose from LibreLinkUp, please restart");
    }
    auto values = doc["data"]["graphData"].as<JsonArray>();
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Got %d glucose values from LibreLinkUp\n", values.size());
#endif
    std::list<GlucoseReading> glucoseReadings;
    glucoseReadings.push_front(readingFromConnection);

    for (JsonVariant v : values) {
        GlucoseReading reading;
        reading.sgv = v["ValueInMgPerDl"].as<int>();
        reading.trend = BG_TREND::NONE;
        String dateString = v["FactoryTimestamp"].as<String>();
        reading.epoch = libreFactoryTimestampToEpoch(dateString);
        if (reading.getSecondsAgo() < 0) {
            DEBUG_PRINTF("Reading from the future: %s: %s\n", dateString.c_str(), reading.toString().c_str());
        }

        glucoseReadings.push_front(reading);
    }
    String debug_read_values = "Read values: ";

    glucoseReadings.sort([](const GlucoseReading &a, const GlucoseReading &b) { return a.epoch < b.epoch; });
    for (auto &reading : glucoseReadings) {
        debug_read_values += String(reading.sgv) + ":(" + String(reading.getSecondsAgo()) + "), ";
    }

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(debug_read_values);
#endif

    doc.clear();
    client->end();

    return glucoseReadings;
}
