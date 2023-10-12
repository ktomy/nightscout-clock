#include "NightscoutManager.h"
#include "globals.h"
#include "SettingsManager.h"
#include "ServerManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <list>
#include <LCBUrl.h>
//#include <StreamUtils.h>

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
    wifiSecureClient = new WiFiClientSecure();
    wifiSecureClient->setInsecure();
    wifiClient = new WiFiClient();
}

void NightscoutManager_::tick() {
    auto currentTime = millis();
    if (lastCallAttemptMills == 0 || currentTime > lastCallAttemptMills + 60*1000UL) {
        getBG(SettingsManager.settings.nsUrl, 36);
        lastCallAttemptMills = currentTime;
    }

}

void NightscoutManager_::getBG(String baseUrl, int numberOfvalues) {

    DEBUG_PRINTLN("Getting NS values...");

    if (baseUrl == "")
    {
        DisplayManager.showFatalError("Nightscout clock is not configured, please go to http://" + ServerManager.myIP.toString() + "/ and configure the device");
    }
    
    LCBUrl url;

    String urlString = String(baseUrl + "api/v1/entries?count=" + numberOfvalues);
    DEBUG_PRINTLN("URL: " + urlString)

    auto urlIsOk = url.setUrl(urlString);
    if (!urlIsOk)
    {
        DisplayManager.showFatalError("Invalid Nightscout URL: " + urlString);
    }

    bool ssl = url.getScheme() == "https";
    DEBUG_PRINTLN(ssl ? "SSL Active" : "No SSL")
    if (ssl)
    {
        client->begin(*wifiSecureClient, url.getHost(), url.getPort(), String("/") + url.getPath() + url.getAfterPath(), true);
    }
    else
    {
        client->begin(url.getHost(), url.getPort(), String("/") + url.getPath() + url.getAfterPath());
    }
    
    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    client->addHeader("Accept", "application/json");
    auto responseCode = client->GET();
    if (responseCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(0xFFFF);

        DeserializationError error = deserializeJson(doc, client->getStream());

        if (error)
        {
            DEBUG_PRINTF("Error deserializing NS response: %s\n", error.c_str());
            if (!firstConnectionSuccess)
            {
                DisplayManager.showFatalError(String("Invalid Nightscout response: ") + error.c_str());
            }

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
            firstConnectionSuccess = true;

            String debugLog = "Received readings: ";
            for(auto &reading : glucoseReadings) {
                debugLog += " " + String(reading.sgv) + " " + String(reading.getSecondsAgo()) + " " + String(reading.trend) + ", ";
            }

            debugLog += "\n";
            DEBUG_PRINTLN(debugLog);
        }
    } else
    {
        DEBUG_PRINTF("Error getting readings %d\n", responseCode);
        if (!firstConnectionSuccess)
        {
            DisplayManager.showFatalError(String("Error connecting to Nightscout: ") + responseCode);
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