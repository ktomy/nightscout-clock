#include "SettingsManager.h"
#include "globals.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// The getter for the instantiated singleton instance
SettingsManager_ &SettingsManager_::getInstance() {
    static SettingsManager_ instance;
    return instance;
}

// Initialize the global shared instance
SettingsManager_ &SettingsManager = SettingsManager.getInstance();

void SettingsManager_::setup() { LittleFS.begin(); }

bool copyFile(const char *srcPath, const char *destPath) {
    File srcFile = LittleFS.open(srcPath, "r");
    if (!srcFile) {
        DEBUG_PRINTLN("Failed to open source file");
        return false;
    }

    File destFile = LittleFS.open(destPath, "w");
    if (!destFile) {
        DEBUG_PRINTLN("Failed to open destination file");
        srcFile.close();
        return false;
    }

    while (srcFile.available()) {
        char data = srcFile.read();
        destFile.write(data);
    }

    srcFile.close();
    destFile.close();

    DEBUG_PRINTLN("File copied successfully");
    return true;
}

void SettingsManager_::factoryReset() {
    copyFile(CONFIG_JSON_FACTORY, CONFIG_JSON);
    LittleFS.end();
    ESP.restart();
}

DynamicJsonDocument *readConfigJsonFile() {
    DynamicJsonDocument *doc;
    if (LittleFS.exists(CONFIG_JSON)) {
        auto settings = Settings();
        File file = LittleFS.open(CONFIG_JSON);
        if (!file || file.isDirectory()) {
            DEBUG_PRINTLN("Failed to open config file for reading");
            return NULL;
        }

        doc = new DynamicJsonDocument((int)(file.size() * 2));
        DeserializationError error = deserializeJson(*doc, file);
        if (error) {
            DEBUG_PRINTF("Deserialization error. File size: %d, requested memory: %d. Error: %s\n", file.size(),
                         (int)(file.size() * 2), error.c_str());
            file.close();
            return NULL;
        }
        return doc;
    } else {
        DEBUG_PRINTLN("Cannot read configuration file");
        return NULL;
    }
}

bool SettingsManager_::loadSettingsFromFile() {
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    settings.ssid = (*doc)["ssid"].as<String>();
    settings.wifi_password = (*doc)["password"].as<String>();
    /// TODO: Get Network config

    settings.nightscout_url = (*doc)["nightscout_url"].as<String>();
    settings.nightscout_api_key = (*doc)["api_secret"].as<String>();
    settings.bg_low_warn_limit = (*doc)["low_mgdl"].as<int>();
    settings.bg_high_warn_limit = (*doc)["high_mgdl"].as<int>();
    settings.bg_units = (*doc)["units"].as<String>() == "mmol" ? BG_UNIT::MMOLL : BG_UNIT::MGDL;
    settings.auto_brightness = (*doc)["auto_brightness"].as<bool>();
    settings.brightness_level = (*doc)["brightness_level"].as<int>() - 1;
    settings.default_clockface = (*doc)["default_face"].as<int>();
    settings.bg_source = (*doc)["data_source"].as<String>() == "nightscout" ? BG_SOURCE::NIGHTSCOUT : BG_SOURCE::DEXCOM;
    settings.dexom_username = (*doc)["dexcom_username"].as<String>();
    settings.dexcom_password = (*doc)["dexcom_password"].as<String>();
    settings.dexcom_server = (*doc)["dexcom_server"].as<String>();

    delete doc;

    this->settings = settings;
    return true;
}

bool SettingsManager_::saveSettingsToFile() {
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    (*doc)["ssid"] = settings.ssid;
    (*doc)["password"] = settings.wifi_password;

    (*doc)["nightscout_url"] = settings.nightscout_url;
    (*doc)["api_secret"] = settings.nightscout_api_key;
    (*doc)["low_mgdl"] = settings.bg_low_warn_limit;
    (*doc)["high_mgdl"] = settings.bg_high_warn_limit;
    (*doc)["units"] = settings.bg_units == BG_UNIT::MMOLL ? "mmol" : "mgdl";
    (*doc)["auto_brightness"] = settings.auto_brightness;
    (*doc)["brightness_level"] = settings.brightness_level + 1;
    (*doc)["default_face"] = settings.default_clockface;
    (*doc)["data_source"] = settings.bg_source == BG_SOURCE::NIGHTSCOUT ? "nightscout" : "dexcom";
    (*doc)["dexcom_username"] = settings.dexom_username;
    (*doc)["dexcom_password"] = settings.dexcom_password;
    (*doc)["dexcom_server"] = settings.dexcom_server;

    if (trySaveJsonAsSettings(*doc) == false)
        return false;

    delete doc;

    return true;
}

bool SettingsManager_::trySaveJsonAsSettings(DynamicJsonDocument doc) {
    DEBUG_PRINTLN(doc.as<String>());
    auto file = LittleFS.open(CONFIG_JSON, FILE_WRITE);
    if (!file) {
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
