#include "ServerManager.h"

#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

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
    WiFiClient client;
    const char* host = "google.com";
    const uint16_t port = 443;
    if (!client.connect(host, port, 1000)) {
        DEBUG_PRINTLN("Internet not reachable");
        client.stop();
        return false;
    }
    DEBUG_PRINTLN("Internet reachable");
    client.stop();
    return true;
}

void ServerManager_::setupWebServer(IPAddress ip) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("ServerManager::setupWebServer");
#endif
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/save", [this](AsyncWebServerRequest* request, JsonVariant& json) {
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

    ws->on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        LittleFS.end();
        ESP.restart();
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/displaypower", [this](AsyncWebServerRequest* request, JsonVariant& json) {
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

    ws->on("/api/factory-reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(1000);
        SettingsManager.factoryReset();
    });

    // api call which returns status (isConnected, internet is reacheable, is in AP mode, bg source type and status)
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
        jsonResponse += "\"}";
        request->send(200, "application/json", jsonResponse);
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
