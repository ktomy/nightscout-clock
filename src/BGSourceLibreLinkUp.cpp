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
}

AuthTicket BGSourceLibreLinkUp::login() {
    // connect and login
    String url = "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/auth/login";

    client->begin(url);
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

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Response: " + response);
#endif

    DynamicJsonDocument doc(0x1400);
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
String BGSourceLibreLinkUp::getLibreLinkUpConnection() {

    // Get connection Id

    return "";
}

#define SHA256_DIGEST_SIZE 32

String encodeSHA256(String toEncode) {
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

    auto glucoseReadings = getReadings(authTicket);

    return glucoseReadings;
}

std::list<GlucoseReading> BGSourceLibreLinkUp::getReadings(AuthTicket authTicket) {

    auto connectionId = getLibreLinkUpConnection();
    if (connectionId == "") {
        DisplayManager.showFatalError("Failed to get connection ID from LibreLinkUp");
    }

    String url =
        "https://" + libreEndpoints[SettingsManager.settings.librelinkup_region] + "/llu/connections/" + connectionId + "/graph";

    client->begin(url);
    for (auto &header : standardHeaders) {
        client->addHeader(header.first, header.second);
    }

    return std::list<GlucoseReading>();
}
