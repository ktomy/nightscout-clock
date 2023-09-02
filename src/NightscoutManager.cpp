#include "NightscoutManager.h"
#include "globals.h"
#include "SettingsManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// The getter for the instantiated singleton instance
NightscoutManager_ &NightscoutManager_::getInstance()
{
    static NightscoutManager_ instance;
    return instance;
}

// Initialize the global shared instance
NightscoutManager_ &NightscoutManager = NightscoutManager.getInstance();



unsigned long lastCallAttemptMills = 0;


void NightscoutManager_::setup() {
    client = new HTTPClient(); 
    transportClient = new WiFiClientSecure();
    transportClient->setInsecure();
}

void NightscoutManager_::tick() {
    auto currentTime = millis();
    if (lastCallAttemptMills == 0 || currentTime > lastCallAttemptMills + 60*1000UL) {
        getBG(SettingsManager.settings.nsHost, SettingsManager.settings.nsPort, 5);
        lastCallAttemptMills = currentTime;
    }

}

void NightscoutManager_::getBG(String server, int port, int numberOfvalues) {
    if (port == 0) {
        port = 443;
    }
    DEBUG_PRINTLN("Getting NS values...");

    String urlPath = String("https://" + server + ":" + port + "/api/v1/entries/sgv?count=" + numberOfvalues);
    DEBUG_PRINTLN("Path: " + urlPath)
    auto connected = client->begin(*transportClient, server, port, urlPath, true);
    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    client->addHeader("Accept", "application/json");
    auto responseCode = client->GET();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, client->getStream());
    String debug_log;
    serializeJsonPretty(doc, debug_log);
    DEBUG_PRINTLN(debug_log);

    client->end();

}