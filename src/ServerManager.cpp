#include "ServerManager.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include "globals.h"
#include "SettingsManager.h"
#include <WiFi.h>
#include <LittleFS.h>
#include "time.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"

// The getter for the instantiated singleton instance
ServerManager_ &ServerManager_::getInstance() {
    static ServerManager_ instance;
    return instance;
}

// Initialize the global shared instance
ServerManager_ &ServerManager = ServerManager.getInstance();

String getID() {
    // uint8_t mac[6];
    // WiFi.macAddress(mac);
    // char *macStr = new char[24];
    // snprintf(macStr, 24, "%s_%02x%02x%02x", HOSTNAME_PREFIX, mac[3], mac[4], mac[5]);
    // return String(macStr);
    return String(HOSTNAME_PREFIX);
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

IPAddress ServerManager_::startWifi(String ssid, String password) {
    this->isInAPMode = false;

    IPAddress ip;
    int timeout = WIFI_CONNECT_TIMEOUT;

    if (ssid != "") {
        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);
        DEBUG_PRINTF("Connecting to %s\n", ssid);

        auto startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(300);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECTED) {
                WiFi.setAutoReconnect(true);
                WiFi.persistent(true);
                ip = WiFi.localIP();
                DEBUG_PRINTLN("Connected");
                failedAttempts = 0; // Reset failed attempts counter
                return ip;
            }
            // If no connection after a while go in Access Point mode
            if (millis() - startTime > timeout)
                break;
        }
    }
    ip = setAPmode(SettingsManager.settings.hostname, AP_MODE_PASSWORD);

    this->isInAPMode = true;

    WiFi.begin();
    return ip;
}

void ServerManager_::reconnectWifi() {
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    myIP = startWifi(SettingsManager.settings.ssid, SettingsManager.settings.wifi_password);
    failedAttempts = 0;
}

AsyncWebHandler ServerManager_::addHandler(AsyncWebHandler *handler) {
    if (ws != nullptr) {
        ws->addHandler(handler);
    }
    return *handler;
}

void ServerManager_::setupWebServer(IPAddress ip) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("ServerManager::setupWebServer");
#endif
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->addHandler(new AsyncCallbackJsonWebHandler("/api/save", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (not json.is<JsonObject>()) {
            request->send(200, "application/json", "{\"status\": \"json parsing error\"}");
            return;
        }
        auto &&data = json.as<JsonObject>();
        if (SettingsManager.trySaveJsonAsSettings(data)) {
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        } else {
            request->send(200, "application/json", "{\"status\": \"Settings save error\"}");
        }
    }));

    ws->addHandler(new AsyncCallbackJsonWebHandler("/api/alarm", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (not json.is<JsonObject>()) {
            request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
            return;
        }
        auto &&data = json.as<JsonObject>();
        if (data["alarmType"].is<String>()) {
            auto alarmType = data["alarmType"].as<String>();
            if (alarmType == "high") {
                PeripheryManager.playRTTTLString(sound_high);
            } else if (alarmType == "low") {
                PeripheryManager.playRTTTLString(sound_low);
            } else if (alarmType == "urgent_low") {
                PeripheryManager.playRTTTLString(sound_urgent_low);
            } else {
                request->send(400, "application/json", "{\"status\": \"alarm type not found\"}");
                return;
            }

            request->send(200, "application/json", "{\"status\": \"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"alarm key not found\"}");
        }
    }));

    ws->on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(200);
        LittleFS.end();
        ESP.restart();
    });

    ws->on("/api/factory-reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(200);
        SettingsManager.factoryReset();
    });

    addStaticFileHandler();

    ws->onNotFound([](AsyncWebServerRequest *request) {
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
        configTime(0, 0, "pool.ntp.org"); // Connect to NTP server, with 0 TZ offset
        if (!getLocalTime(&timeinfo)) {
            DEBUG_PRINTLN("Failed to obtain time");
            return false;
        }

        lastTimeSync = getUtcEpoch();

        setTimezone();

        timeinfo = getTimezonedTime();

        DEBUG_PRINTF("Timezone is: %s, local time is: %02d.%02d.%d %02d:%02d:%02d\n",
                     SettingsManager.settings.tz_libc_value.c_str(), timeinfo.tm_mday, timeinfo.tm_mon + 1,
                     timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        Serial.printf("Local time is: %02d.%02d.%d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1,
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
    auto hostname = SettingsManager.settings.hostname = getID();
    WiFi.setHostname(hostname.c_str()); // define hostname

    myIP = startWifi(SettingsManager.settings.ssid, SettingsManager.settings.wifi_password);

    auto ipAP = IPAddress();
    ipAP.fromString(AP_IP);
    isConnected = myIP != ipAP;
    DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);

    setupWebServer(myIP);
    setTimezone();
}

void ServerManager_::tick() {
    initTimeIfNeeded();
    if (apMode)
        dnsServer.processNextRequest();
}
