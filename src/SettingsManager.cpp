#include "SettingsManager.h"
#include <Arduino.h>
#include "globals.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// The getter for the instantiated singleton instance
SettingsManager_ &SettingsManager_::getInstance()
{
    static SettingsManager_ instance;
    return instance;
}

// Initialize the global shared instance
SettingsManager_ &SettingsManager = SettingsManager.getInstance();

void SettingsManager_::begin()
{
    LittleFS.begin();
}


void SettingsManager_::loadSettingsFromFile()
{
    if (LittleFS.exists("/config.json"))
    {
        auto settings = Settings();
        File file = LittleFS.open(CONFIG_JSON, "r");
        DynamicJsonDocument doc(file.size() * 1.33);
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            DEBUG_PRINTLN(error.c_str());
            return;
        }
        settings.ssid = doc["ssid"].as<String>();
        settings.password = doc["password"].as<String>();

        ///TODO: Get Network config

        settings.nsUrl = doc["nightscout_url"].as<String>();
        settings.bgLow = doc["low_mgdl"].as<int>();
        settings.bgHigh = doc["high_mgdl"].as<int>();

        this->settings = settings;

    }
    else
    {
        DEBUG_PRINTLN("Cannot read configuration file");
    }

}
bool SettingsManager_::trySaveJsonAsSettings(JsonObject json)
{
    auto doc = DynamicJsonDocument(json);
    DEBUG_PRINTLN(doc.as<String>());
    return false;

}
