#include "NightscoutManager.h"
#include "globals.h"
#include "SettingsManager.h"
#include "ServerManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <list>

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
        getBG(SettingsManager.settings.nsHost, SettingsManager.settings.nsPort, 10);
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
    if (responseCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, client->getStream());
        if (error)
        {
            DEBUG_PRINTF("Error deserializing NS response: %s\n", error.c_str());
            return;
        }

        std::list<GlucoseReading> lastReadings;
        if (doc.is<JsonArray>()) {
            JsonArray jsonArray = doc.as<JsonArray>();
            for (JsonVariant v : jsonArray) {
                GlucoseReading reading;
                reading.sgv = v["sgv"].as<int>();
                reading.epoch = v["date"].as<unsigned long long>() / 1000;
                // sensor.epoch = v["mills"].as<unsigned long>();
                reading.trend = (BG_TREND)(v["trend"].as<int>());
                lastReadings.push_front(reading);
            }

            glucoseReadings = lastReadings;
            lastReadingEpoch = glucoseReadings.back().epoch;

            String debugLog = "Received readings: ";
            for(auto &reading : glucoseReadings) {
                debugLog += " " + String(reading.sgv) + " " + String(reading.getSecondsAgo()) + " " + String(reading.trend) + ", ";
            }

            debugLog += "\n";
            DEBUG_PRINTLN(debugLog);
        }
    }

    client->end();
}

    bool NightscoutManager_::hasNewData(unsigned long long epochToCompare) {
        return lastReadingEpoch > epochToCompare;

    }
    std::list<GlucoseReading> NightscoutManager_::getGlucoseData() {
        return glucoseReadings;
    }