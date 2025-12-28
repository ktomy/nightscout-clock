
#include "BGSourceMedtrum.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DisplayManager.h>
#include <ServerManager.h>
#include <SettingsManager.h>

#define MEDTRUM_LOGIN_URL "https://easyview.medtrum.eu/mobile/ajax/login"
#define MEDTRUM_MONITOR_URL "https://easyview.medtrum.eu/mobile/ajax/monitor?flag=monitor_list"
#define MEDTRUM_HISTORY_URL "https://easyview.medtrum.eu/mobile/ajax/download?flag=sg"

void BGSourceMedtrum::setup() {
    BGSource::setup();
    status = "initialized";
    sessionCookie = "";
}

bool BGSourceMedtrum::login() {
    String email = SettingsManager.settings.medtrum_email;
    String password = SettingsManager.settings.medtrum_password;
    if (email == "" || password == "") {
        status = "not_configured";
        DisplayManager.showFatalError(
            "Medtrum credentials not set, go to http://" + ServerManager.myIP.toString() +
            "/ to configure");

        return false;
    }
    client->begin(*wifiSecureClient, MEDTRUM_LOGIN_URL);
    client->setTimeout(15000);
    client->addHeader("DevInfo", "Android 12;Xiamoi vayu;Android 12");
    client->addHeader("AppTag", "v=1.2.70(112);n=eyfo;p=android");
    client->addHeader("User-Agent", "okhttp/3.5.0");
    client->addHeader("Content-Type", "application/x-www-form-urlencoded");
    String body =
        "apptype=Follow&user_name=" + email + "&password=" + password + "&platform=google&user_type=M";

    const char* headerKeys[] = {"Set-Cookie"};
    client->collectHeaders(headerKeys, 1);

    int responseCode = client->POST(body);
    if (responseCode != HTTP_CODE_OK) {
        status = "login_failed";
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError("Medtrum login failed: HTTP " + String(responseCode));
        } else {
            handleFailedAttempt();
        }

        client->end();
        return false;
    }

    String response = client->getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error || doc["res"].as<String>() != "OK") {
        status = "login_failed";
        String detail = doc["resdetail"].isNull() ? "Unknown error" : doc["resdetail"].as<String>();
        DisplayManager.showFatalError("Medtrum login failed: " + detail);
        client->end();
        sessionCookie = "";
        return false;
    }

    sessionCookie = client->header("Set-Cookie");

    if (sessionCookie == "") {
        status = "login_failed";
        DisplayManager.showFatalError("Medtrum login failed: No session cookie received");
        client->end();
        sessionCookie = "";
        return false;
    }
    client->end();
    return true;
}

BG_TREND parseTrendFromGlucoseRate(int glucoseRate) {
    BG_TREND trend = BG_TREND::NONE;
    switch (glucoseRate) {
        case 0:
        case 8:
            trend = BG_TREND::FLAT;
            break;
        case 1:
            trend = BG_TREND::FORTY_FIVE_UP;
            break;
        case 2:
            trend = BG_TREND::SINGLE_UP;
            break;
        case 3:
            trend = BG_TREND::DOUBLE_UP;
            break;
        case 4:
            trend = BG_TREND::FORTY_FIVE_DOWN;
            break;
        case 5:
            trend = BG_TREND::SINGLE_DOWN;
            break;
        case 6:
            trend = BG_TREND::DOUBLE_DOWN;
            break;
        default:
            trend = BG_TREND::NONE;
            break;
    }
    return trend;
}

GlucoseReading BGSourceMedtrum::getCurrentGlucose(String& username, bool& ok) {
    GlucoseReading reading;
    ok = false;
    client->begin(*wifiSecureClient, MEDTRUM_MONITOR_URL);
    client->addHeader("DevInfo", "Android 12;Xiamoi vayu;Android 12");
    client->addHeader("AppTag", "v=1.2.70(112);n=eyfo;p=android");
    client->addHeader("User-Agent", "okhttp/3.5.0");
    client->addHeader("Cookie", sessionCookie);
    DEBUG_PRINTLN("Cookie: " + sessionCookie);
    int responseCode = client->GET();
    if (responseCode != HTTP_CODE_OK) {
        status = "get_glucose_failed";
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError("Medtrum get glucose failed: HTTP " + String(responseCode));
        } else {
            handleFailedAttempt();
        }
        client->end();
        return reading;
    }
    String response = client->getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error || doc["res"].as<String>() != "OK") {
        status = "get_glucose_failed";
        String detail = doc["resdetail"].isNull() ? "Unknown error" : doc["resdetail"].as<String>();
        DisplayManager.showFatalError("Medtrum get glucose failed: " + detail);
        client->end();
        return reading;
    }
    double glucose = doc["monitorlist"][0]["sensor_status"]["glucose"].as<double>();
    int glucoseRate = doc["monitorlist"][0]["sensor_status"]["glucoseRate"].as<int>();
    username = doc["monitorlist"][0]["username"].as<String>();
    unsigned long long glucoseTimestamp =
        doc["monitorlist"][0]["sensor_status"]["updateTime"].as<unsigned long long>();
    int glucosemgdl;
    if (glucose < 30.0) {
        glucosemgdl = int(glucose * 18.0);
    } else {
        glucosemgdl = int(glucose);
    }

    auto trend = parseTrendFromGlucoseRate(glucoseRate);

    reading.sgv = glucosemgdl;
    reading.trend = trend;
    reading.epoch = glucoseTimestamp;
    client->end();
    ok = true;
    return reading;
}

std::list<GlucoseReading> BGSourceMedtrum::getHistory(
    const String& username, time_t from, time_t to, bool& ok) {
    std::list<GlucoseReading> readings;
    ok = false;
    char st[32], et[32];
    strftime(st, sizeof(st), "%Y-%m-%d%%20%H:%M:%S", gmtime(&from));
    strftime(et, sizeof(et), "%Y-%m-%d%%20%H:%M:%S", gmtime(&to));
    String url = String(MEDTRUM_HISTORY_URL) + "&st=" + st + "&et=" + et + "&user_name=" + username;
    client->begin(*wifiSecureClient, url);
    client->addHeader("DevInfo", "Android 12;Xiamoi vayu;Android 12");
    client->addHeader("AppTag", "v=1.2.70(112);n=eyfo;p=android");
    client->addHeader("User-Agent", "okhttp/3.5.0");
    client->addHeader("Cookie", sessionCookie);
    int responseCode = client->GET();
    if (responseCode != HTTP_CODE_OK) {
        status = "get_history_failed";
        if (!firstConnectionSuccess) {
            DisplayManager.showFatalError("Medtrum get history failed: HTTP " + String(responseCode));
        } else {
            handleFailedAttempt();
        }
        client->end();
        return readings;
    }
    String response = client->getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error || doc["res"].as<String>() != "OK") {
        status = "get_history_failed";
        String detail = doc["resdetail"].isNull() ? "Unknown error" : doc["resdetail"].as<String>();
        DisplayManager.showFatalError("Medtrum get history failed: " + detail);
        client->end();
        return readings;
    }
    for (JsonVariant entry : doc["data"].as<JsonArray>()) {
        unsigned long long ts = entry[1].as<unsigned long long>();
        double g = entry[3].as<double>();
        int mgdl;
        if (g < 30.0)
            mgdl = int(g * 18.0);
        else
            mgdl = int(g);
        GlucoseReading r;
        r.sgv = mgdl;
        r.trend = BG_TREND::NONE;
        r.epoch = ts;
        readings.push_back(r);
    }
    client->end();
    ok = true;
    return readings;
}

std::list<GlucoseReading> BGSourceMedtrum::updateReadings(std::list<GlucoseReading> existingReadings) {
    // Only login if not already logged in
    if (!isLoggedIn()) {
        if (!login()) {
            return existingReadings;
        }
    }

    // Get current glucose
    String username;
    bool ok = false;
    GlucoseReading currentReading = getCurrentGlucose(username, ok);
    if (!ok) {
        return existingReadings;
    }

    // Determine if we need to fetch history
    bool needHistory = false;
    unsigned long long lastEpoch = existingReadings.size() > 0 ? existingReadings.back().epoch : 0;
    if (existingReadings.size() == 0 || (currentReading.epoch - lastEpoch) > 5 * 60) {
        needHistory = true;
    }

    std::list<GlucoseReading> newReadings;

    if (needHistory) {
        if (lastEpoch == 0) {
            lastEpoch = currentReading.epoch - BG_BACKFILL_SECONDS;
        }
        // we will cycle through getting history in pirces of 1h unles we fill the gap
        // We create an array of intervals to fetch and call getHistory for each interval
        time_t from = lastEpoch + 1;
        time_t to = from + 60 * 60;  // 1 hour intervals
        time_t now = time(NULL);
        while (from < currentReading.epoch) {
            if (to > now) {
                to = now;
            }
            std::list<GlucoseReading> intervalReadings = getHistory(username, from, to, ok);
            if (!ok) {
                break;
            }
            newReadings.insert(newReadings.end(), intervalReadings.begin(), intervalReadings.end());
            from = to + 1;
            to = from + 60 * 60;
        }
    }

    newReadings.push_back(currentReading);

    // Remove duplicates with existingReadings
    newReadings.remove_if([&existingReadings](const GlucoseReading& reading) {
        return std::find_if(
                   existingReadings.begin(), existingReadings.end(),
                   [&reading](const GlucoseReading& existingReading) {
                       return existingReading.epoch == reading.epoch;
                   }) != existingReadings.end();
    });

    if (newReadings.size() == 0) {
        return existingReadings;
    }

    // Add new readings to existing
    existingReadings.insert(existingReadings.end(), newReadings.begin(), newReadings.end());
    existingReadings.sort(
        [](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });
    status = "connected";
    firstConnectionSuccess = true;
    return existingReadings;
}
