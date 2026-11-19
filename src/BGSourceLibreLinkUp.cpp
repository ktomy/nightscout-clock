#include "BGSourceLibreLinkUp.h"

#include <Arduino.h>
#include <AsyncJson.h>

#include "LibreLinkUpTimestamp.h"

#define TIMESTAMP_FIELD "FactoryTimestamp"

bool BGSourceLibreLinkUp::hasValidAuthentication() {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Checking if authentication is valid, token: %s, expires: %llu, now: %llu\n",
        authTicket.token.c_str(), authTicket.expires, (unsigned long long)time(NULL));
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
    // The connections request keeps its connection open for the next poll, and the server keeps an
    // idle one for several minutes. A new connection needs a TLS handshake, which takes a second or
    // two of the main loop and about 45 KB of memory at its peak, and was done every minute. A kept
    // connection holds about 41 KB the whole time instead, and a request on it takes a tenth of a
    // second and about 5 KB.
    //
    // The login has a connection of its own, closed after its answer: in 1.1.0 the connections
    // request went out on the login's connection and, for one account, never got an answer (#253).
    // If requests on a kept connection fail MAX_KEPT_CONNECTION_FAILURES times in a row, every
    // request gets a new connection, as before.
    client->setReuse(keepConnection);
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

    // A connection of its own (see setup()).
    closeConnection();
    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000);  // Set timeout to 15 seconds (15000 milliseconds)
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    String body = "{\"email\":\"" + SettingsManager.settings.librelinkup_email + "\",\"password\":\"" +
                  SettingsManager.settings.librelinkup_password + "\"}";

    logNetworkAndMemoryStats("before", "LibreLinkUp login");
    auto responseCode = client->POST(body);
    logNetworkAndMemoryStats("after", "LibreLinkUp login");
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
    closeConnection();

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

    // Status 4 = server demands a step (currently Terms-of-Use re-accept).
    // Seen since the LibreLinkUp 5.0.0 rollout. Accept the step, then reboot
    // into a clean login. Note: the status-4 response carries a short-lived
    // authTicket but NO user.id, so there is no account-id to send — the
    // continue endpoint validates the Bearer token only.
    if (doc["status"].as<int>() == 4) {
        String stepType = doc["data"]["step"]["type"].as<String>();
        String stepToken = doc["data"]["authTicket"]["token"].as<String>();
        DEBUG_PRINTF("Login requires step: %s\n", stepType.c_str());

        String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] +
                     "/auth/continue/" + stepType;

        client->begin(*wifiSecureClient, url);
        client->setTimeout(15000);
        for (auto& header : standardHeaders) {
            client->addHeader(header.first, header.second);
        }
        client->addHeader("Authorization", "Bearer " + stepToken);

        logNetworkAndMemoryStats("before", "LibreLinkUp login step");
        auto stepResponseCode = client->POST("{}");
        logNetworkAndMemoryStats("after", "LibreLinkUp login step");
        DEBUG_PRINTF("Step accept response: %d\n", stepResponseCode);
        closeConnection();

        if (stepResponseCode == HTTP_CODE_OK) {
            DisplayManager.clearMatrix();
            DisplayManager.setTextColor(COLOR_CYAN);
            DisplayManager.printText(0, 6, "Accept", TEXT_ALIGNMENT::CENTER, 0);
            DisplayManager.update();
            delay(2000);
            ESP.restart();
        }
        status = "login_step_failed";
        DisplayManager.showFatalError("LibreLinkUp requires action in the LibreLinkUp app");
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
        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_CYAN);
        DisplayManager.printText(0, 6, "Redirect", TEXT_ALIGNMENT::CENTER, 0);
        DisplayManager.update();
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

void BGSourceLibreLinkUp::closeConnection() {
    client->end();
    // end() keeps the connection open when reuse is on.
    wifiSecureClient->stop();
}

int BGSourceLibreLinkUp::requestConnections(const String& url) {
    // Use the shared secure client like the login and graph requests do. begin(url) built a new
    // TLS client on every poll and dropped the shared one each time.
    client->begin(*wifiSecureClient, url);
    client->setTimeout(15000);  // Set timeout to 15 seconds (15000 milliseconds)
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    logNetworkAndMemoryStats("before", "LibreLinkUp connection");
    auto responseCode = client->GET();
    logNetworkAndMemoryStats("after", "LibreLinkUp connection");
    return responseCode;
}

GlucoseReading BGSourceLibreLinkUp::getLibreLinkUpConnection() {
    String url =
        "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/connections";

    // connected() also finds out whether the server has closed the kept connection.
    bool kept = wifiSecureClient->connected();
    auto responseCode = requestConnections(url);
    if (kept) {
        if (responseCode > 0) {
            keptConnectionFailures = 0;
        } else {
            // No answer on the kept connection, for example because it broke without the server
            // closing it. Try once more on a new one.
            keptConnectionFailures++;
            DEBUG_PRINTF(
                "LibreLinkUp request on the kept connection failed with %d (failure %d), trying a "
                "new connection\n",
                responseCode, keptConnectionFailures);
            if (keptConnectionFailures >= MAX_KEPT_CONNECTION_FAILURES) {
                DEBUG_PRINTLN("LibreLinkUp: a new connection for every request from now on");
                keepConnection = false;
                client->setReuse(false);
            }
            closeConnection();
            responseCode = requestConnections(url);
        }
    }

    // A failed request or an unreadable answer is usually a passing network or server problem.
    // Keep the readings the clock has and try again on the next poll; show an error only after
    // MAX_RETRY_COUNT failures in a row, or for an HTTP error before the first connection.
    if (responseCode != HTTP_CODE_OK) {
        retryCount += 1;
#ifdef DEBUG_BG_SOURCE
        // The body of an HTTP error, such as a 429 or 5xx page, is usually not JSON, so it is
        // only logged.
        String errorBody = client->getString();
        DEBUG_PRINTF(
            "Error getting connections from LibreLinkUp %d (failure %d): %.200s\n", responseCode,
            retryCount, errorBody.c_str());
#endif
        // Not kept: the error's body may still be unread.
        closeConnection();
        if (!firstConnectionSuccess || retryCount > MAX_RETRY_COUNT) {
            status = "get_connections_failed";
            DisplayManager.showFatalError(
                String("Error getting connections from LibreLinkUp: ") + String(responseCode));
        }
        return GlucoseReading();  // return empty reading to indicate no data
    }

    String response = client->getString();
    client->end();

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Connection Response: " + response);
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        // Empty, cut short or not JSON
        retryCount += 1;
        DEBUG_PRINTF(
            "Error deserializing LibreLinkUp connections response (failure %d): %s\n"
            "Failed on string: %s\n",
            retryCount, error.c_str(), response.c_str());
        if (retryCount <= MAX_RETRY_COUNT) {
            return GlucoseReading();  // return empty reading to indicate no data
        }
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
    reading.epoch = LibreLinkUpTimestamp::toEpoch(
        doc["data"][patientIndex]["glucoseMeasurement"][TIMESTAMP_FIELD].as<const char*>());
    reading.trend =
        trendFromLibreTrend(doc["data"][patientIndex]["glucoseMeasurement"]["TrendArrow"].as<int>());

    retryCount = 0;
    firstConnectionSuccess = true;
    status = "connected";

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

void BGSourceLibreLinkUp::updateReadings(GlucoseReadings& readings) {
    if (!hasValidAuthentication()) {
        deleteAuthTicket();
        auto newAuthTicket = login();
        if (newAuthTicket.token == "") {
            status = "login_failed";
            DisplayManager.showFatalError("Failed to login to LibreLinkUp, check credentials");
        }

        authTicket = newAuthTicket;
    }

    unsigned long long lastReadingEpoch = readings.size() > 0 ? readings.back().epoch : 0;

    // Readings the clock already has are left out.
    if (readings.addAll(getReadings(lastReadingEpoch)) == 0) {
        DEBUG_PRINTLN("No new readings");
        return;
    }

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN(
        "Existing readings: " + String(readings.size()) +
        ", last reading epoch: " + String(readings.back().epoch) +
        " Difference to now is: " + String((time(NULL) - readings.back().epoch) / 60) + " minutes");

    // print 5 last readings if they exist in format: "SGV - minutes ago, trend, "
    if (readings.size() > 0) {
        String debugLog = "Received readings: ";
        int count = 0;
        for (auto it = readings.rbegin(); it != readings.rend() && count < 5; ++it, ++count) {
            debugLog += " " + String(it->sgv) + " -" + String(it->getSecondsAgo() / 60) + "m " +
                        toString(it->trend) + ", ";
        }

        debugLog += "\n";
        DEBUG_PRINTLN(debugLog);
    }
#endif
}

std::list<GlucoseReading> BGSourceLibreLinkUp::getReadings(unsigned long long lastReadingEpoch) {
    auto readingFromConnection = getLibreLinkUpConnection();

    // Compare with the newest stored reading, not with the time of the last poll: a reading
    // reaches LibreLinkUp some time after the sensor takes it, often after the clock has polled.
    if (readingFromConnection.isEmpty() || lastReadingEpoch >= readingFromConnection.epoch) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTF("No readings needed, last reading epoch %llu is already in\n", lastReadingEpoch);
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
    // The graph answer is about 30 KB. getString() needed twice that in free memory, and when
    // memory ran out or the connection dropped it returned the body cut short, without an error.
    // The JSON is parsed straight from the connection instead, keeping only the fields in the
    // filter. HTTP/1.0 keeps the server from sending the body in chunks, whose size lines would
    // break the JSON.
    client->useHTTP10(true);
    client->setTimeout(15000);  // Set timeout to 15 seconds (15000 milliseconds)
    for (auto& header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    client->addHeader("Authorization", "Bearer " + authTicket.token);
    client->addHeader("account-id", encodeSHA256(authTicket.accountId));

    JsonDocument filter;
    filter["data"]["graphData"][0]["ValueInMgPerDl"] = true;
    filter["data"]["graphData"][0][TIMESTAMP_FIELD] = true;
    filter["status"] = true;

    DeserializationOption::Filter filterOption(filter);

    JsonDocument doc;
    DeserializationError error = DeserializationError::Ok;

    logNetworkAndMemoryStats("before", "LibreLinkUp graph");
    auto responseCode = client->GET();
    logNetworkAndMemoryStats("after", "LibreLinkUp graph");
    int responseSize = client->getSize();  // -1 without a Content-Length header
    if (responseCode == HTTP_CODE_OK) {
        error = deserializeJson(doc, client->getStream(), filterOption);
    }
    // HTTP/1.0 closes the connection after the answer.
    client->end();
    // Back to HTTP/1.1 for the other requests. useHTTP10(false) also turns connection reuse on.
    client->useHTTP10(false);
    client->setReuse(keepConnection);

    // The graph only fills in the history. If it fails, keep the current reading from the
    // connections request; the next gap of more than five minutes requests the graph again.
    if (responseCode != HTTP_CODE_OK) {
        DEBUG_PRINTF(
            "Error getting graph from LibreLinkUp %d, using the current reading only\n", responseCode);
        return std::list<GlucoseReading>{readingFromConnection};
    }

    if (error) {
        DEBUG_PRINTF(
            "Error deserializing LibreLinkUp glucose response (Content-Length %d): %s, using the "
            "current reading only\n",
            responseSize, error.c_str());
        return std::list<GlucoseReading>{readingFromConnection};
    }

    if (doc["status"].as<int>() != 0) {
        DEBUG_PRINTF(
            "Failed to get glucose from LibreLinkUp, non-zero status %d, using the current reading "
            "only\n",
            doc["status"].as<int>());
        return std::list<GlucoseReading>{readingFromConnection};
    }
    auto values = doc["data"]["graphData"].as<JsonArray>();
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF("Got %d glucose values from LibreLinkUp\n", values.size());
#endif
    std::list<GlucoseReading> glucoseReadings;
    glucoseReadings.push_front(readingFromConnection);

    // The graph covers about 12 hours, but only the last 3 are kept: older readings were added here
    // and deleted on the next poll, leaving a hole in the heap for each.
    long long now = time(NULL);
    unsigned long long oldestKept = now > (BG_BACKFILL_SECONDS) ? now - (BG_BACKFILL_SECONDS) : 0;
    for (JsonVariant v : values) {
        const char* dateString = v[TIMESTAMP_FIELD].as<const char*>();
        GlucoseReading reading;
        reading.sgv = v["ValueInMgPerDl"].as<int>();
        reading.trend = BG_TREND::NONE;
        reading.epoch = LibreLinkUpTimestamp::toEpoch(dateString);

        if (reading.epoch < oldestKept) {
            continue;
        }
        if (reading.getSecondsAgo() < 0) {
            DEBUG_PRINTF(
                "Reading from the future: %s: %s: %lu\n", dateString, reading.toString().c_str(),
                time(NULL));
        }

        glucoseReadings.push_front(reading);
    }

    glucoseReadings.sort(
        [](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });

#ifdef DEBUG_BG_SOURCE
    String debug_read_values = "Read values: ";
    for (auto& reading : glucoseReadings) {
        debug_read_values += String(reading.sgv) + ":(" + String(reading.getSecondsAgo()) + "), ";
    }
    DEBUG_PRINTLN(debug_read_values);
#endif

    doc.clear();

    return glucoseReadings;
}
