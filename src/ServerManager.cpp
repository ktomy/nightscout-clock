#include "ServerManager.h"

#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_system.h>

#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "SettingsManager.h"
#include "globals.h"
#include "time.h"

// The getter for the instantiated singleton instance
ServerManager_& ServerManager_::getInstance() {
    static ServerManager_ instance;
    return instance;
}

static String resolveAlarmMelody(const String& alarmType) {
    if (alarmType == "high") {
        if (SettingsManager.settings.alarm_high_melody.length() > 0) {
            return SettingsManager.settings.alarm_high_melody;
        }
        return sound_high;
    }
    if (alarmType == "low") {
        if (SettingsManager.settings.alarm_low_melody.length() > 0) {
            return SettingsManager.settings.alarm_low_melody;
        }
        return sound_low;
    }
    if (alarmType == "urgent_low") {
        if (SettingsManager.settings.alarm_urgent_low_melody.length() > 0) {
            return SettingsManager.settings.alarm_urgent_low_melody;
        }
        return sound_urgent_low;
    }

    return "";
}

// Web authentication constants, cookie set for 10 minutes
static constexpr unsigned long WEB_AUTH_TOKEN_TTL_MS = 10UL * 60UL * 1000UL;
static constexpr int WEB_AUTH_COOKIE_MAX_AGE_SEC = 10 * 60;
static const char* WEB_AUTH_COOKIE_NAME = "auth_token";

static String getCookieValue(const String& cookieHeader, const String& name) {
    int pos = 0;
    while (pos < cookieHeader.length()) {
        int end = cookieHeader.indexOf(';', pos);
        if (end < 0) {
            end = cookieHeader.length();
        }
        String pair = cookieHeader.substring(pos, end);
        pair.trim();
        int eq = pair.indexOf('=');
        if (eq > 0) {
            String key = pair.substring(0, eq);
            key.trim();
            if (key == name) {
                return pair.substring(eq + 1);
            }
        }
        pos = end + 1;
    }
    return "";
}

static String buildAuthCookie(const String& token, int maxAgeSeconds) {
    String cookie = String(WEB_AUTH_COOKIE_NAME) + "=" + token;
    if (maxAgeSeconds >= 0) {
        cookie += "; Max-Age=" + String(maxAgeSeconds);
    }
    cookie += "; Path=/; SameSite=Strict; HttpOnly";
    return cookie;
}

// Initialize the global shared instance
ServerManager_& ServerManager = ServerManager.getInstance();

String ServerManager_::getHostname() {
    if (SettingsManager.settings.hostname != "") {
        return SettingsManager.settings.hostname;
    }

    String hostname = HOSTNAME_PREFIX;

    if (SettingsManager.settings.custom_hostname_enable) {
        hostname = SettingsManager.settings.custom_hostname;
    }

    SettingsManager.settings.hostname = hostname;

    DEBUG_PRINTF("Hostname: %s\n", hostname.c_str());

    return hostname;
}

bool ServerManager_::isWebAuthEnabled() const {
    return SettingsManager.settings.web_auth_enable &&
           SettingsManager.settings.web_auth_password.length() > 0;
}

bool ServerManager_::isRequestAuthenticated(AsyncWebServerRequest* request) const {
    if (!isWebAuthEnabled()) {
        return false;
    }

    if (webAuthToken.length() == 0) {
        return false;
    }

    if (webAuthTokenIssuedMs == 0 || (millis() - webAuthTokenIssuedMs) > WEB_AUTH_TOKEN_TTL_MS) {
        const_cast<ServerManager_*>(this)->webAuthToken = "";
        const_cast<ServerManager_*>(this)->webAuthTokenIssuedMs = 0;
        return false;
    }

    String requestToken = "";
    if (request->hasHeader("Cookie")) {
        requestToken = getCookieValue(request->getHeader("Cookie")->value(), WEB_AUTH_COOKIE_NAME);
    }

    return requestToken.length() > 0 && requestToken == webAuthToken;
}

String ServerManager_::generateAuthToken() {
    uint32_t part1 = esp_random();
    uint32_t part2 = esp_random();
    uint32_t part3 = millis();
    return String(part1, HEX) + String(part2, HEX) + String(part3, HEX);
}

bool ServerManager_::enforceAuthentication(AsyncWebServerRequest* request) {
    if (!isWebAuthEnabled()) {
        return true;
    }

    if (isRequestAuthenticated(request)) {
        return true;
    }

    request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
    return false;
}

IPAddress ServerManager_::setAPmode(String ssid, String psk) {
    auto ipAP = IPAddress();
    auto netmaskAP = IPAddress();
    auto gatewayAP = IPAddress();
    ipAP.fromString(AP_IP);
    netmaskAP.fromString(AP_NETMASK);
    gatewayAP.fromString(AP_GATEWAY);

    apMode = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.softAPConfig(ipAP, gatewayAP, netmaskAP);
    WiFi.softAP(ssid, psk);
    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    return WiFi.softAPIP();
}

void initiateWiFiConnection(String wifi_type, String ssid, String username, String password) {
    if (wifi_type == "wpa_eap") {
        WiFi.begin(ssid, WPA2_AUTH_PEAP, username, username, password);
    } else if (password == "") {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, password);
    }
}

bool tryConnectToWiFi(String wifi_type, String ssid, String username, String password) {
    if (ssid == "") {
        DEBUG_PRINTLN("SSID is empty, cannot connect to WiFi");
        return false;
    }
    int timeout = WIFI_CONNECT_TIMEOUT;

    WiFi.mode(WIFI_STA);

    DEBUG_PRINTF("Connecting to %s (%s)\n", ssid.c_str(), wifi_type.c_str());

    initiateWiFiConnection(wifi_type, ssid, username, password);

    auto startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        // If no connection after a while go in Access Point mode
        if (millis() - startTime > timeout)
            break;
    }

    Serial.println();

    return false;
}

IPAddress ServerManager_::startWifi() {
    this->isInAPMode = false;

    IPAddress ip;

    bool connected = tryConnectToWiFi(
        "wpa_psk", SettingsManager.settings.ssid, "", SettingsManager.settings.wifi_password);
    if (!connected && SettingsManager.settings.additional_wifi_enable) {
        connected = tryConnectToWiFi(
            SettingsManager.settings.additional_wifi_type, SettingsManager.settings.additional_wifi_ssid,
            SettingsManager.settings.additional_wifi_username,
            SettingsManager.settings.additional_wifi_password);
    }

    if (connected) {
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        ip = WiFi.localIP();
        DEBUG_PRINTLN("Connected");
        failedAttempts = 0;  // Reset failed API call attempts counter
        return ip;
    }

    DEBUG_PRINTLN("Failed to connect to WiFi, starting AP mode");

    ip = setAPmode(getHostname(), AP_MODE_PASSWORD);
    this->isInAPMode = true;
    WiFi.begin();
    return ip;
}

// When we cannot get BG for some time, we want to reconnect to wifi
void ServerManager_::reconnectWifi() {
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    myIP = startWifi();
    failedAttempts = 0;
}

AsyncWebHandler ServerManager_::addHandler(AsyncWebHandler* handler) {
    if (ws != nullptr) {
        ws->addHandler(handler);
    }
    return *handler;
}

bool canReachInternet() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return false;
    }

    WiFiClient client;
    const char* host = "www.google.com";
    const uint16_t port = 80;
    if (!client.connect(host, port, 2000)) {
        DEBUG_PRINTLN("Internet not reachable (connect failed)");
        client.stop();
        return false;
    }

    // Send a simple HTTP GET request
    client.print("GET /generate_204 HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");

    unsigned long start = millis();
    while (client.connected() && !client.available() && millis() - start < 2000) {
        delay(10);
    }

    if (!client.available()) {
        DEBUG_PRINTLN("Internet not reachable (no response)");
        client.stop();
        return false;
    }

    // Optionally, check for HTTP/1.1 204 No Content response
    String line = client.readStringUntil('\n');
    if (line.indexOf("204") > 0 || line.indexOf("200") > 0) {
        // DEBUG_PRINTLN("Internet reachable");
        client.stop();
        return true;
    }

    DEBUG_PRINTLN("Internet not reachable (bad response)");
    client.stop();
    return false;
}

void ServerManager_::setupWebServer(IPAddress ip) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("ServerManager::setupWebServer");
#endif
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->on("/api/auth/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"enabled\": ";
        jsonResponse += isWebAuthEnabled() ? "true" : "false";
        jsonResponse += ", \"authenticated\": ";
        jsonResponse += isRequestAuthenticated(request) ? "true" : "false";
        jsonResponse += ", \"hasPassword\": ";
        jsonResponse += SettingsManager.settings.web_auth_password.length() > 0 ? "true" : "false";
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/auth/login", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            if (!isWebAuthEnabled()) {
                request->send(200, "application/json", "{\"status\": \"disabled\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            String password = data["password"].as<String>();
            if (password != SettingsManager.settings.web_auth_password) {
                request->send(401, "application/json", "{\"status\": \"invalid\"}");
                return;
            }

            webAuthToken = generateAuthToken();
            webAuthTokenIssuedMs = millis();
            String jsonResponse = "{\"status\": \"ok\"}";
            auto response = request->beginResponse(200, "application/json", jsonResponse);
            response->addHeader(
                "Set-Cookie", buildAuthCookie(webAuthToken, WEB_AUTH_COOKIE_MAX_AGE_SEC));
            request->send(response);
        }));

    ws->on("/api/auth/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!isWebAuthEnabled()) {
            request->send(200, "application/json", "{\"status\": \"disabled\"}");
            return;
        }
        if (!isRequestAuthenticated(request)) {
            request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
            return;
        }
        webAuthToken = "";
        webAuthTokenIssuedMs = 0;
        auto response = request->beginResponse(200, "application/json", "{\"status\": \"ok\"}");
        response->addHeader("Set-Cookie", buildAuthCookie("", 0));
        request->send(response);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/save", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(200, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (SettingsManager.trySaveJsonAsSettings(data)) {
                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(200, "application/json", "{\"status\": \"Settings save error\"}");
            }
        }));

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/alarm/custom", [](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            if (!data["rtttl"].is<String>()) {
                request->send(400, "application/json", "{\"status\": \"rtttl key not found\"}");
                return;
            }

            auto melody = data["rtttl"].as<String>();
            melody.trim();

            if (melody.length() == 0 || melody.indexOf(':') < 0) {
                request->send(400, "application/json", "{\"status\": \"invalid melody\"}");
                return;
            }

            PeripheryManager.playRTTTLString(melody);
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        }));

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/alarm", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (data["alarmType"].is<String>()) {
                auto alarmType = data["alarmType"].as<String>();
                auto melody = resolveAlarmMelody(alarmType);
                if (melody == "") {
                    request->send(400, "application/json", "{\"status\": \"alarm type not found\"}");
                    return;
                }

                PeripheryManager.playRTTTLString(melody);

                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\": \"alarm key not found\"}");
            }
        }));

    ws->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        LittleFS.end();
        ESP.restart();
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/displaypower", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (data["power"].is<String>()) {
                auto powerState = data["power"].as<String>();
                if (powerState == "on") {
                    DisplayManager.setPower(true);
                } else if (powerState == "off") {
                    DisplayManager.setPower(false);
                } else {
                    request->send(400, "application/json", "{\"status\": \"Invalid power state\"}");
                    return;
                }

                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\": \"power key not found\"}");
            }
        }));

    ws->on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        SettingsManager.factoryReset();
    });

    // api call which returns status (isConnected, internet is reacheable, is in AP mode, bg source type
    // and status)
    ws->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"isConnected\": ";
        jsonResponse += this->isConnected ? "true" : "false";
        jsonResponse += ", \"hasInternet\": ";
        jsonResponse += canReachInternet() ? "true" : "false";
        jsonResponse += ", \"isInAPMode\": ";
        jsonResponse += this->isInAPMode ? "true" : "false";
        jsonResponse += ", \"bgSource\": \"";
        jsonResponse += toString(bgSourceManager.getCurrentSourceType());
        jsonResponse += "\", \"bgSourceStatus\": \"";
        jsonResponse += bgSourceManager.getSourceStatus();
        jsonResponse += "\", \"sgv\": ";
        auto glucoseData = bgSourceManager.getGlucoseData();
        if (!glucoseData.empty()) {
            jsonResponse += String(glucoseData.back().sgv);
        } else {
            jsonResponse += "0";
        }
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    ws->on("/config.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(LittleFS, CONFIG_JSON, "application/json");
    });

    addStaticFileHandler();

    ws->onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    ws->begin();
}

void ServerManager_::removeStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        ws->removeHandler(staticFilesHandler);
        staticFilesHandler = nullptr;
    } else {
        DEBUG_PRINTLN("removeStaticFileHandler: staticFilesHandler is null");
    }
}

void ServerManager_::addStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        removeStaticFileHandler();
    }
    staticFilesHandler = new AsyncStaticWebHandler("/", LittleFS, "/", NULL);
    staticFilesHandler->setDefaultFile("index.html");
    ws->addHandler(staticFilesHandler);
}

bool ServerManager_::initTimeIfNeeded() {
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo) || getUtcEpoch() - lastTimeSync > TIME_SYNC_INTERVAL) {
        configTime(0, 0, "pool.ntp.org");  // Connect to NTP server, with 0 TZ offset
        if (!getLocalTime(&timeinfo)) {
            DEBUG_PRINTLN("Failed to obtain time");
            return false;
        }

        lastTimeSync = getUtcEpoch();

        setTimezone();

        timeinfo = getTimezonedTime();

        DEBUG_PRINTF(
            "Timezone is: %s, local time is: %02d.%02d.%d %02d:%02d:%02d\n",
            SettingsManager.settings.tz_libc_value.c_str(), timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        Serial.printf(
            "Local time is: %02d.%02d.%d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return true;
}

void ServerManager_::setTimezone() {
    setenv("TZ", SettingsManager.settings.tz_libc_value.c_str(), 1);
    tzset();
}

unsigned long ServerManager_::getUtcEpoch() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
        return 0;
    } else {
        auto utc = time(nullptr);
        return utc;
    }
}

tm ServerManager_::getTimezonedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
    }
    return timeinfo;
}

void ServerManager_::stop() {
    ws->end();
    delete ws;
    WiFi.disconnect();
}

void ServerManager_::setup() {
    WiFi.setHostname(getHostname().c_str());  // define hostname

    myIP = startWifi();

    auto ipAP = IPAddress();
    ipAP.fromString(AP_IP);
    DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);

    setupWebServer(myIP);
    setTimezone();

    isConnected = myIP != ipAP;
}

void ServerManager_::tick() {
    if (apMode) {
        dnsServer.processNextRequest();
    } else {
        initTimeIfNeeded();
    }
}
