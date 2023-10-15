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

bool copyFile(const char *srcPath, const char *destPath)
{
    File srcFile = LittleFS.open(srcPath, "r");
    if (!srcFile)
    {
        DEBUG_PRINTLN("Failed to open source file");
        return false;
    }

    File destFile = LittleFS.open(destPath, "w");
    if (!destFile)
    {
        DEBUG_PRINTLN("Failed to open destination file");
        srcFile.close();
        return false;
    }

    while (srcFile.available())
    {
        char data = srcFile.read();
        destFile.write(data);
    }

    srcFile.close();
    destFile.close();

    DEBUG_PRINTLN("File copied successfully");
    return true;
}

void SettingsManager_::factoryReset()
{
    copyFile(CONFIG_JSON_FACTORY, CONFIG_JSON);
    LittleFS.end();
    ESP.restart();
}

DynamicJsonDocument *readConfigJsonFile()
{
    DynamicJsonDocument *doc;
    if (LittleFS.exists(CONFIG_JSON))
    {
        auto settings = Settings();
        File file = LittleFS.open(CONFIG_JSON);
        if (!file || file.isDirectory())
        {
            DEBUG_PRINTLN("Failed to open config file for reading");
            return NULL;
        }

        doc = new DynamicJsonDocument((int)(file.size() * 2));
        DeserializationError error = deserializeJson(*doc, file);
        if (error)
        {
            DEBUG_PRINTF("Deserialization error. File size: %d, requested memory: %d. Error: %s\n", file.size(), (int)(file.size() * 2), error.c_str());
            file.close();
            return NULL;
        }
        return doc;
    }
    else
    {
        DEBUG_PRINTLN("Cannot read configuration file");
        return NULL;
    }
}

bool SettingsManager_::loadSettingsFromFile()
{
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    settings.ssid = (*doc)["ssid"].as<String>();
    settings.password = (*doc)["password"].as<String>();
    /// TODO: Get Network config

    settings.nsUrl = (*doc)["nightscout_url"].as<String>();
    settings.nsApiKey = (*doc)["api_secret"].as<String>();
    settings.bgLow = (*doc)["low_mgdl"].as<int>();
    settings.bgHigh = (*doc)["high_mgdl"].as<int>();
    settings.bgUnit = (*doc)["units"].as<String>() == "mmol" ? MMOLL : MGDL;

    delete doc;

    this->settings = settings;
    return true;
}

bool SettingsManager_::saveSettingsToFile()
{
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    (*doc)["ssid"] = settings.ssid;
    (*doc)["password"] = settings.password;

    (*doc)["nightscout_url"] = settings.nsUrl;
    (*doc)["api_secret"] = settings.nsApiKey;
    (*doc)["low_mgdl"] = settings.bgLow;
    (*doc)["high_mgdl"] = settings.bgHigh;
    (*doc)["units"] = settings.bgUnit == MMOLL ? "mmol" : "mgdl";

    if (trySaveJsonAsSettings(*doc) == false)
        return false;

    delete doc;

    return true;
}

bool SettingsManager_::trySaveJsonAsSettings(DynamicJsonDocument doc)
{
    DEBUG_PRINTLN(doc.as<String>());
    auto file = LittleFS.open(CONFIG_JSON, FILE_WRITE);
    if (!file)
    {
        DEBUG_PRINTLN("Failed to open config file for writing");
        return false;
    }

    auto result = file.print(doc.as<String>());

    file.close();
    if (!result)
    {
        return false;
    }

    return true;
}
