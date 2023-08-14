#include "ServerManager.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include "Globals.h"
#include "SettingsManager.h"
#include <WiFi.h>
#include <LittleFS.h>

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
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char *macStr = new char[24];
    snprintf(macStr, 24, "%s_%02x%02x%02x", HOSTNAME_PREFIX, mac[3], mac[4], mac[5]);
    return String(macStr);
}

void ServerManager_::begin()
{

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
    WiFi.mode(WIFI_STA);
    IPAddress ip;
    int timeout = WIFI_CONNECT_TIMEOUT;
    WiFi.mode(WIFI_STA);

    if (ssid.length() > 0 && password.length() > 0)
    {
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

    WiFi.begin();
    return ip;

}

void ServerManager_::setupWebServer(IPAddress ip)
{
    DEBUG_PRINTLN("Setting up web server 2");
    ws = new AsyncWebServer(80);
    // ws->serveStatic("/index.html", LittleFS, "/index.html");
    // ws->serveStatic("/favicon.ico", LittleFS, "/favicon.ico");
    // ws->serveStatic("/script.js", LittleFS, "/script.js");
    // ws->serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
    // ws->serveStatic("/config.json", LittleFS, "/config.json");


ws->addHandler(new AsyncCallbackJsonWebHandler(
  "/api/save",
  [this](AsyncWebServerRequest* request, JsonVariant& json) {
    DEBUG_PRINTLN("We are here");
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


    //   if (not data["name"].is<String>()) {
    //     request->send(400, "text/plain", "name is not a string");
    //     return;
    //   }
    //   String name = data["name"].as<String>();

    //   if (name == "IDLE") {
    //     set_mode_idle(request, data); // handle data and respond
    //   } else if (name == "RANDOM") {
    //     set_mode_random(request, data); // handle data and respond
    //   } else {
    //    request->send(400, "text/plain", "Invalid mode");
    //   }
}));

    ws->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");;


    // Send a POST request to <IP>/post with a form field message set to <message>
    // ws->on("/saveconfig", HTTP_POST, [](AsyncWebServerRequest *request){
    //     String message;
    //     if (SettingsManager.trySaveStringAsSettings(request->arg("plain")))
    //     {
    //         request->send(200, "application/json", "{\"status\": \"ok\"}");
    //     }
    //     else
    //     {
    //         request->send(200, "application/json", "{\"status\": \"error\"}");
    //     }
    // });

    ws->begin();
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
    //ws->handleClient();
    if (apMode)
        dnsServer.processNextRequest();
}

