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

// The getter for the instantiated singleton instance
ServerManager_ &ServerManager_::getInstance()
{
    static ServerManager_ instance;
    return instance;
}

// Initialize the global shared instance
ServerManager_ &ServerManager = ServerManager.getInstance();

String getID()
{
    // uint8_t mac[6];
    // WiFi.macAddress(mac);
    // char *macStr = new char[24];
    // snprintf(macStr, 24, "%s_%02x%02x%02x", HOSTNAME_PREFIX, mac[3], mac[4], mac[5]);
    // return String(macStr);
    return String(HOSTNAME_PREFIX);
}

IPAddress ServerManager_::setAPmode(String ssid, String psk)
{
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

IPAddress ServerManager_::startWifi(String ssid, String password)
{
    this->isInAPMode = false;

    IPAddress ip;
    int timeout = WIFI_CONNECT_TIMEOUT;

    if (ssid != "")
    {
        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);
        DEBUG_PRINTF("Connecting to %s\n", ssid);

        auto startTime = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(300);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECTED)
            {
                WiFi.setAutoReconnect(true);
                WiFi.persistent(true);
                ip = WiFi.localIP();
                DEBUG_PRINTLN("Connected");
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

void ServerManager_::setupWebServer(IPAddress ip)
{
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/save",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            if (not json.is<JsonObject>())
            {
                request->send(200, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto &&data = json.as<JsonObject>();
            if (SettingsManager.trySaveJsonAsSettings(data))
            {
                request->send(200, "application/json", "{\"status\": \"ok\"}");
            }
            else
            {
                request->send(200, "application/json", "{\"status\": \"Settings save error\"}");
            }
        }));

    ws->on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request)
           {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(200);
        LittleFS.end();
        ESP.restart(); });

    ws->on("/api/factory-reset", HTTP_POST, [](AsyncWebServerRequest *request)
           {
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        delay(200);
        SettingsManager.factoryReset(); });

    ws->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    ;

    ws->onNotFound([](AsyncWebServerRequest *request)
                   {
    if (request->method() == HTTP_OPTIONS) {
        request->send(200);
    } else {
        request->send(404);
    } });

    ws->begin();
}

bool initTimeIfNeeded()
{

    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        configTime(0, 0, "pool.ntp.org"); // Connect to NTP server, with 0 TZ offset
        if (!getLocalTime(&timeinfo))
        {
            DEBUG_PRINTLN("Failed to obtain time");
            return false;
        }

        DEBUG_PRINTF("Got the time from NTP: %02d.%02d.%d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return true;
}

unsigned long ServerManager_::getTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        DEBUG_PRINTLN("Failed to obtain time");
        return 0;
    }
    else
    {
        time_t t = mktime(&timeinfo);
        return t;
    }
}

void ServerManager_::stop()
{
    ws->end();
    delete ws;
    WiFi.disconnect();
}

void ServerManager_::setup()
{
    auto hostname = SettingsManager.settings.hostname = getID();
    WiFi.setHostname(hostname.c_str()); // define hostname

    myIP = startWifi(SettingsManager.settings.ssid, SettingsManager.settings.password);

    auto ipAP = IPAddress();
    ipAP.fromString(AP_IP);
    isConnected = myIP != ipAP;
    DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);

    setupWebServer(myIP);
}

void ServerManager_::tick()
{
    initTimeIfNeeded();
    if (apMode)
        dnsServer.processNextRequest();
}
