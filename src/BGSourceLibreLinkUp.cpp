#include "BGSourceLibreLinkUp.h"

#include <Arduino.h>
#include <AsyncJson.h>

#define TIMESTAMP_FIELD "FactoryTimestamp"

bool BGSourceLibreLinkUp::hasValidAuthentication() {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Checking if authentication is valid, token: %s, expires: %llu, now: %llu\n",
        authTicket.token.c_str(), authTicket.expires, time(NULL));
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

void BGSourceLibreLinkUp::setup() {
    BGSource::setup();
    ServerManager.removeStaticFileHandler();

    // add handler to retrieve patients list

    ServerManager.addHandler(new AsyncCallbackJsonWebHandler(
        "/api/llu/patients", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            String response = "[";
            bool first = true;
            for (const auto& patient : this->patients) {
                if (!first)
                    response += ",";
                response += "{";
                response += "\"patientId\":\"" + patient.patientId + "\",";
                response += "\"firstName\":\"" + patient.firstName + "\",";
                response += "\"lastName\":\"" + patient.lastName + "\"";
                response += "}";
                first = false;
            }
            response += "]";
            request->send(200, "application/json", response);
        }));

    ServerManager.addStaticFileHandler();
}

AuthTicket BGSourceLibreLinkUp::login() {
    if (libreEndpoints.find(SettingsManager.settings.librelinkup_region) == libreEndpoints.end()) {
        DEBUG_PRINTF("Invalid region %s\n", SettingsManager.settings.librelinkup_region.c_str());
        DisplayManager.showFatalError("Invalid LibreLinkUp region, please check settings");
    }

    String url =
        "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/auth/login";

    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000);  // Set timeout to 15 seconds (15000 milliseconds)
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    String body = "{\"email\":\"" + SettingsManager.settings.librelinkup_email + "\",\"password\":\"" +
                  SettingsManager.settings.librelinkup_password + "\"}";

    auto responseCode = client->POST(body);
    if (responseCode != HTTP_CODE_OK &&
        (firstConnectionSuccess == false || retryCount > MAX_RETRY_COUNT)) {
        DEBUG_PRINTF("Error logging in to LibreLinkUp %d\n", responseCode);
        status = "login_failed";
        DisplayManager.showFatalError(
            String("Error logging in to LibreLinkUp: ") + String(responseCode));
    }
    if (responseCode != HTTP_CODE_OK) {
        retryCount += 1;
    } else {
        retryCount = 0;
    }

    String response = client->getString();

    // #ifdef DEBUG_BG_SOURCE
    //     DEBUG_PRINTLN("Response: " + response);
    // #endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        DEBUG_PRINTF(
            "Error deserializing LibreLinkUp login response: %s\nFailed on string: %s\n", error.c_str(),
            response.c_str());
        status = "login_failed";
        DisplayManager.showFatalError(String("Invalid LibreLinkUp login response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF("Failed to login to LibreLinkUp, non-zero status %s\n", response.c_str());
        status = "login_failed";
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
        DisplayManager.printText(0, 6, "Redirect", TEXT_ALIGNMENT::CENTER, 0);
        delay(2000);
        ESP.restart();
    }

    String token = doc["data"]["authTicket"]["token"].as<String>();
    unsigned long long expires = doc["data"]["authTicket"]["expires"].as<unsigned long long>();
    unsigned long long duration = doc["data"]["authTicket"]["duration"].as<unsigned long long>();
    String accountId = doc["data"]["user"]["id"].as<String>();
    auto authTicket = AuthTicket(token, expires, duration, accountId);

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Logged in to LibreLinkUp, token: %s, expires: %llu, duration: %llu, accountId %s\n",
        token.c_str(), expires, duration, accountId.c_str());
#endif

    return authTicket;
}

unsigned long long BGSourceLibreLinkUp::libreTimestampToEpoch(String dateString) {
    setenv("TZ", "UTC0", 1);
    tzset();
    struct tm tm;
    strptime(dateString.c_str(), "%m/%d/%Y", &tm);
    // parse the time part without using strptime to avoid AM/PM bug
    String timePart = dateString.substring(dateString.indexOf(' ') + 1);
    int hour = timePart.substring(0, timePart.indexOf(':')).toInt();
    int minute = timePart.substring(timePart.indexOf(':') + 1, timePart.lastIndexOf(':')).toInt();
    int second = timePart.substring(timePart.lastIndexOf(':') + 1, timePart.indexOf(' ')).toInt();
    String amPm = timePart.substring(timePart.indexOf(' ') + 1);
    // adjust hour for AM/PM
    if (amPm == "PM" && hour < 12) {
        hour += 12;  // convert PM hour to 24-hour format
    } else if (amPm == "AM" && hour == 12) {
        hour = 0;  // convert 12 AM to 0 hours
    }
    // set the time components in the tm structure
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Value to parse: %s, Parsed time: %02d:%02d:%02d, date: %02d/%02d/%04d\n", dateString.c_str(),
        tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
#endif
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
    String url =
        "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/connections";

    client->begin(url);
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    auto responseCode = client->GET();

    if (responseCode != HTTP_CODE_OK &&
        (firstConnectionSuccess == false || retryCount > MAX_RETRY_COUNT)) {
        DEBUG_PRINTF("Error getting connections from LibreLinkUp %d\n", responseCode);
        status = "get_connections_failed";
        DisplayManager.showFatalError(
            String("Error getting connections from LibreLinkUp: ") + String(responseCode));
    }
    if (responseCode != HTTP_CODE_OK) {
        retryCount += 1;
    } else {
        retryCount = 0;
    }

    String response = client->getString();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Connection Response: " + response);
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        if (error == DeserializationError::EmptyInput) {
            DEBUG_PRINTLN("Empty response from LibreLinkUp, no data to process");
            retryCount += 1;
            if (retryCount <= MAX_RETRY_COUNT) {
                return GlucoseReading();  // return empty reading to indicate no data
            }
        }
        DEBUG_PRINTF(
            "Error deserializing LibreLinkUp connections response: %s\nFailed on string: %s\n",
            error.c_str(), response.c_str());
        status = "get_connections_failed";
        DisplayManager.showFatalError(
            String("Invalid LibreLinkUp connections response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF(
            "Failed to get connections from LibreLinkUp, non-zero status %s\n", response.c_str());
        status = "get_connections_failed";
        DisplayManager.showFatalError("Failed to get connections from LibreLinkUp, please restart");
    }

    int patientsCount = doc["data"].size();

    if (patientsCount == 0) {
        DEBUG_PRINTLN("No LibreLinkUp connections found");
        status = "no_connections";
        DisplayManager.showFatalError(
            "No LibreLinkUp connections/patients found. Add followers in the LibreLink Up app");
    }

    int patientIndex = 0;
    bool patientFound = false;
    patients.clear();

    // find patient index by patient id
    for (int i = 0; i < patientsCount; i++) {
        String patientId = doc["data"][i]["patientId"].as<String>();
        String firstName = doc["data"][i]["firstName"].as<String>();
        String lastName = doc["data"][i]["lastName"].as<String>();
        patients.push_back({patientId, firstName, lastName});

        if (SettingsManager.settings.librelinkup_patient_id != "" &&
            patientId == SettingsManager.settings.librelinkup_patient_id) {
            patientIndex = i;
            patientFound = true;
        }
    }

    if (!patientFound && patientsCount > 1) {
        DEBUG_PRINTF(
            "Configured patient id \"%s\" not found among %d patients\n",
            SettingsManager.settings.librelinkup_patient_id.c_str(), patientsCount);
        status = "multiple_patients_no_match";
        DisplayManager.showFatalError(
            "Multiple patients detected, go to http://" + ServerManager.myIP.toString() +
            "/ to select a patient");
    }

    authTicket.patientId = doc["data"][patientIndex]["patientId"].as<String>();

    GlucoseReading reading;
    reading.sgv = doc["data"][patientIndex]["glucoseMeasurement"]["ValueInMgPerDl"].as<int>();
    String dateString = doc["data"][patientIndex]["glucoseMeasurement"][TIMESTAMP_FIELD].as<String>();
    reading.epoch = libreTimestampToEpoch(dateString);
    reading.trend =
        trendFromLibreTrend(doc["data"][patientIndex]["glucoseMeasurement"]["TrendArrow"].as<int>());

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Got LibreLinkUp connection, patientId: %s, last reading: %s\n",
        authTicket.patientId.c_str(), reading.toString().c_str());
#endif

    return reading;
}

#define SHA256_DIGEST_SIZE 32

String BGSourceLibreLinkUp::encodeSHA256(String toEncode) {
    unsigned char hash[SHA256_DIGEST_SIZE];
    mbedtls_sha256((const unsigned char*)toEncode.c_str(), toEncode.length(), hash, 0);
    String encoded = "";
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        encoded += buf;
    }
    return encoded;
}

std::list<GlucoseReading> BGSourceLibreLinkUp::updateReadings(
    std::list<GlucoseReading> existingReadings) {
    if (!hasValidAuthentication()) {
        deleteAuthTicket();
        auto newAuthTicket = login();
        if (newAuthTicket.token == "") {
            status = "login_failed";
            DisplayManager.showFatalError("Failed to login to LibreLinkUp, check credentials");
        }

        authTicket = newAuthTicket;
    }

    unsigned long long lastReadingEpoch =
        existingReadings.size() > 0 ? existingReadings.back().epoch : 0;

    auto glucoseReadings = getReadings(lastReadingEpoch);

    // sort the readings by epoch
    glucoseReadings.sort(
        [](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });

    // remove readings from retrievedReadings which are already present in existingReadings
    // because some servers (Gluroo) don't process from-to in an expected way
    glucoseReadings.remove_if([&existingReadings](const GlucoseReading& reading) {
        return std::find_if(
                   existingReadings.begin(), existingReadings.end(),
                   [&reading](const GlucoseReading& existingReading) {
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
    DEBUG_PRINTLN(
        "Existing readings: " + String(existingReadings.size()) +
        ", last reading epoch: " + String(existingReadings.back().epoch) + " Difference to now is: " +
        String((time(NULL) - existingReadings.back().epoch) / 60) + " minutes");

    // print 5 last readings if they exist in format: "SGV - minutes ago, trend, "
    if (existingReadings.size() > 0) {
        String debugLog = "Received readings: ";
        int count = 0;
        for (auto it = existingReadings.rbegin(); it != existingReadings.rend() && count < 5;
             ++it, ++count) {
            debugLog += " " + String(it->sgv) + " -" + String(it->getSecondsAgo() / 60) + "m " +
                        toString(it->trend) + ", ";
        }

        debugLog += "\n";
        DEBUG_PRINTLN(debugLog);
    }
#endif

    return existingReadings;
}

std::list<GlucoseReading> BGSourceLibreLinkUp::getReadings(unsigned long long lastReadingEpoch) {
    auto readingFromConnection = getLibreLinkUpConnection();

    if (readingFromConnection.isEmpty() || lastCallAttemptEpoch >= readingFromConnection.epoch) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF(
            "No readings needed, last reading epoch %llu is already in\n", lastCallAttemptEpoch);
#endif
        return std::list<GlucoseReading>();
    }

    if (lastReadingEpoch >= readingFromConnection.epoch - 60 * 5) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF(
            "No historical data needed, last reading is %d seconds older then the current reading\n",
            readingFromConnection.epoch - lastReadingEpoch);
#endif
        return std::list<GlucoseReading>{readingFromConnection};
    }

    String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] +
                 "/llu/connections/" + authTicket.patientId + "/graph";

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Getting glucose from LibreLinkUp, url: %s\n", url.c_str());
#endif

    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000);  // Set timeout to 15 seconds (15000 milliseconds)
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    auto responseCode = client->GET();
    if (responseCode != HTTP_CODE_OK &&
        (firstConnectionSuccess == false || retryCount > MAX_RETRY_COUNT)) {
        DEBUG_PRINTF("Error getting graph from LibreLinkUp %d\n", responseCode);
        status = "get_glucose_failed";
        DisplayManager.showFatalError(
            String("Error getting graph from LibreLinkUp: ") + String(responseCode));
    }
#ifdef DEBUG_BG_SOURCE
    else {
        if (responseCode != HTTP_CODE_OK) {
            DEBUG_PRINTF("Error getting graph from LibreLinkUp %d\n", responseCode);
        }
    }
#endif
    if (responseCode != HTTP_CODE_OK) {
        retryCount += 1;
    } else {
        retryCount = 0;
    }
    String response = client->getString();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Graph Response: " + response);
#endif

    firstConnectionSuccess = true;
    status = "connected";

    JsonDocument doc;

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Free memory: %d\n", ESP.getFreeHeap());
#endif

    JsonDocument filter;
    filter["data"]["graphData"][0]["ValueInMgPerDl"] = true;
    filter["data"]["graphData"][0][TIMESTAMP_FIELD] = true;
    filter["status"] = true;

    DeserializationOption::Filter filterOption(filter);

    DeserializationError error = deserializeJson(doc, response, filterOption);
    // DeserializationError error = deserializeJson(doc, response);
    if (error) {
        if (error == DeserializationError::EmptyInput) {
            DEBUG_PRINTLN("Empty response from LibreLinkUp, no data to process");
            retryCount += 1;
            if (retryCount <= MAX_RETRY_COUNT) {
                return std::list<GlucoseReading>{readingFromConnection};
            }
        }
        DEBUG_PRINTF(
            "Error deserializing LibreLinkUp glucose response: %s\nFailed on string: %s\n",
            error.c_str(), response.c_str());
        status = "get_glucose_failed";
        DisplayManager.showFatalError(String("Invalid LibreLinkUp glucose response: ") + error.c_str());
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF("Failed to get glucose from LibreLinkUp, non-zero status %s\n", response.c_str());
        status = "get_glucose_failed";
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
        String dateString = v[TIMESTAMP_FIELD].as<String>();
        reading.epoch = libreTimestampToEpoch(dateString);
        // DEBUG_PRINTF(
        //     "Reading date: %s, epoch: %llu, seconds ago: %d\n", dateString.c_str(), reading.epoch,
        //     reading.getSecondsAgo());

        if (reading.getSecondsAgo() < 0) {
            DEBUG_PRINTF(
                "Reading from the future: %s: %s: %lu\n", dateString.c_str(), reading.toString().c_str(),
                time(NULL));
        }

        glucoseReadings.push_front(reading);
    }
    String debug_read_values = "Read values: ";

    glucoseReadings.sort(
        [](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });
    for (auto& reading : glucoseReadings) {
        debug_read_values += String(reading.sgv) + ":(" + String(reading.getSecondsAgo()) + "), ";
    }

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(debug_read_values);
#endif

    doc.clear();
    client->end();

    return glucoseReadings;
}
