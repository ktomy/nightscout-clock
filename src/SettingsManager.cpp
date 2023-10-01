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

void SettingsManager_::setup()
{
    LittleFS.begin();
}


bool SettingsManager_::loadSettingsFromFile()
{
    if (LittleFS.exists(CONFIG_JSON))
    {
        auto settings = Settings();
        File file = LittleFS.open(CONFIG_JSON);
        if (!file || file.isDirectory()) {
            DEBUG_PRINTLN("Failed to open config file for reading");
            return false;
        }
        DynamicJsonDocument doc((int)(file.size() * 2));
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            DEBUG_PRINTF("Deserialization error. File size: %d, requested memory: %d. Error: %s\n", file.size(), (int)(file.size() * 2), error.c_str());
            file.close();
            return false;
        }
        settings.ssid = doc["ssid"].as<String>();
        settings.password = doc["password"].as<String>();

        ///TODO: Get Network config

        settings.nsUrl = doc["nightscout_url"].as<String>();
        settings.nsApiKey = doc["api_secret"].as<String>();
        settings.bgLow = doc["low_mgdl"].as<int>();
        settings.bgHigh = doc["high_mgdl"].as<int>();
        settings.bgUnit = doc["units"].as<String>() == "mmol" ? MMOLL : MGDL;

        this->settings = settings;
        return true;
    }
    else
    {
        DEBUG_PRINTLN("Cannot read configuration file");
        return false;
    }
}

bool SettingsManager_::trySaveJsonAsSettings(JsonObject json)
{
    auto doc = DynamicJsonDocument(json);
    DEBUG_PRINTLN(doc.as<String>());
    auto file = LittleFS.open(CONFIG_JSON, FILE_WRITE);
    if(!file){
        DEBUG_PRINTLN("Failed to open config file for writing");
        return false;
    }

    auto result = file.print(doc.as<String>());

    file.close();
    if (!result) {
        return false;
    }

    return true;
}
