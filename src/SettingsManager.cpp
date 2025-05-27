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

JsonDocument *SettingsManager_::readConfigJsonFile() {
    JsonDocument *doc;
    if (LittleFS.exists(CONFIG_JSON)) {
        auto settings = Settings();
        File file = LittleFS.open(CONFIG_JSON);
        if (!file || file.isDirectory()) {
            DEBUG_PRINTLN("Failed to open config file for reading");
            return NULL;
        }

        doc = new JsonDocument();
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
        factoryReset();
        return NULL;
    }
}

bool SettingsManager_::loadSettingsFromFile() {
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    settings.ssid = (*doc)["ssid"].as<String>();
    settings.wifi_password = (*doc)["password"].as<String>();

    settings.bg_low_warn_limit = (*doc)["low_mgdl"].as<int>();
    settings.bg_high_warn_limit = (*doc)["high_mgdl"].as<int>();
    settings.bg_low_urgent_limit = (*doc)["low_urgent_mgdl"].as<int>();
    settings.bg_high_urgent_limit = (*doc)["high_urgent_mgdl"].as<int>();
    settings.bg_units = (*doc)["units"].as<String>() == "mmol" ? BG_UNIT::MMOLL : BG_UNIT::MGDL;

    settings.auto_brightness = (*doc)["auto_brightness"].as<bool>();
    settings.brightness_level = (*doc)["brightness_level"].as<int>() - 1;
    settings.default_clockface = (*doc)["default_face"].as<int>();

    String data_source = (*doc)["data_source"].as<String>();
    if (data_source == "nightscout") {
        settings.bg_source = BG_SOURCE::NIGHTSCOUT;
    } else if (data_source == "dexcom") {
        settings.bg_source = BG_SOURCE::DEXCOM;
    } else if (data_source == "medtronic") {
        settings.bg_source = BG_SOURCE::MEDTRONIC;
    } else if (data_source == "api") {
        settings.bg_source = BG_SOURCE::API;
    } else if (data_source == "librelinkup") {
        settings.bg_source = BG_SOURCE::LIBRELINKUP;
    } else {
        settings.bg_source = BG_SOURCE::NO_SOURCE;
    }
    settings.dexcom_username = (*doc)["dexcom_username"].as<String>();
    settings.dexcom_password = (*doc)["dexcom_password"].as<String>();
    settings.dexcom_server = (*doc)["dexcom_server"].as<String>() == "us" ? DEXCOM_SERVER::US : DEXCOM_SERVER::NON_US;

    settings.librelinkup_email = (*doc)["librelinkup_email"].as<String>();
    settings.librelinkup_password = (*doc)["librelinkup_password"].as<String>();
    settings.librelinkup_region = (*doc)["librelinkup_region"].as<String>();

    settings.nightscout_url = (*doc)["nightscout_url"].as<String>();
    settings.nightscout_api_key = (*doc)["api_secret"].as<String>();

    settings.tz_libc_value = (*doc)["tz_libc"].as<String>();
    settings.time_format = (*doc)["time_format"].as<String>() == "12" ? TIME_FORMAT::HOURS_12 : TIME_FORMAT::HOURS_24;

    // read alarms data
    settings.alarm_urgent_low_enabled = (*doc)["alarm_urgent_low_enabled"].as<bool>();
    settings.alarm_urgent_low_mgdl = (*doc)["alarm_urgent_low_value"].as<int>();
    settings.alarm_urgent_low_snooze_minutes = (*doc)["alarm_urgent_low_snooze_interval"].as<int>();
    settings.alarm_urgent_low_silence_interval = (*doc)["alarm_urgent_low_silence_interval"].as<String>();
    settings.alarm_low_enabled = (*doc)["alarm_low_enabled"].as<bool>();
    settings.alarm_low_mgdl = (*doc)["alarm_low_value"].as<int>();
    settings.alarm_low_snooze_minutes = (*doc)["alarm_low_snooze_interval"].as<int>();
    settings.alarm_low_silence_interval = (*doc)["alarm_low_silence_interval"].as<String>();
    settings.alarm_high_enabled = (*doc)["alarm_high_enabled"].as<bool>();
    settings.alarm_high_mgdl = (*doc)["alarm_high_value"].as<int>();
    settings.alarm_high_snooze_minutes = (*doc)["alarm_high_snooze_interval"].as<int>();
    settings.alarm_high_silence_interval = (*doc)["alarm_high_silence_interval"].as<String>();

    // Additional WiFi
    settings.additional_wifi_enable = (*doc)["additional_wifi_enable"].as<bool>();
    settings.additional_wifi_type = (*doc)["additional_wifi_type"].as<String>();
    settings.additional_wifi_ssid = (*doc)["additional_ssid"].as<String>();
    settings.additional_wifi_username = (*doc)["additional_wifi_username"].as<String>();
    settings.additional_wifi_password = (*doc)["additional_wifi_password"].as<String>();

    // Custom hostname
    settings.custom_hostname_enable = (*doc)["custom_hostname_enable"].as<bool>();
    settings.custom_hostname = (*doc)["custom_hostname"].as<String>();

    // Custom No Data Timer
    settings.custom_nodatatimer_enable = (*doc)["custom_nodatatimer_enable"].as<bool>();
    settings.custom_nodatatimer = (*doc)["custom_nodatatimer"].as<int>();
    BG_DATA_OLD_OFFSET_MINUTES = settings.custom_nodatatimer;  // Update global variable
    
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

    (*doc)["low_mgdl"] = settings.bg_low_warn_limit;
    (*doc)["high_mgdl"] = settings.bg_high_warn_limit;
    (*doc)["low_urgent_mgdl"] = settings.bg_low_urgent_limit;
    (*doc)["high_urgent_mgdl"] = settings.bg_high_urgent_limit;

    (*doc)["units"] = settings.bg_units == BG_UNIT::MMOLL ? "mmol" : "mgdl";

    (*doc)["auto_brightness"] = settings.auto_brightness;
    (*doc)["brightness_level"] = settings.brightness_level + 1;
    (*doc)["default_face"] = settings.default_clockface;

    String data_source = "no_source";
    switch (settings.bg_source) {
        case BG_SOURCE::NIGHTSCOUT:
            data_source = "nightscout";
            break;
        case BG_SOURCE::DEXCOM:
            data_source = "dexcom";
            break;
        case BG_SOURCE::MEDTRONIC:
            data_source = "medtronic";
            break;
        case BG_SOURCE::API:
            data_source = "api";
            break;
        case BG_SOURCE::LIBRELINKUP:
            data_source = "librelinkup";
            break;
        default:
            data_source = "no_source";
            break;
    }
    (*doc)["data_source"] = data_source;

    (*doc)["dexcom_username"] = settings.dexcom_username;
    (*doc)["dexcom_password"] = settings.dexcom_password;
    (*doc)["dexcom_server"] = settings.dexcom_server == DEXCOM_SERVER::US ? "us" : "ous";

    (*doc)["librelinkup_email"] = settings.librelinkup_email;
    (*doc)["librelinkup_password"] = settings.librelinkup_password;
    (*doc)["librelinkup_region"] = settings.librelinkup_region;

    (*doc)["nightscout_url"] = settings.nightscout_url;
    (*doc)["api_secret"] = settings.nightscout_api_key;

    (*doc)["tz_libc"] = settings.tz_libc_value;
    (*doc)["time_format"] = settings.time_format == TIME_FORMAT::HOURS_12 ? "12" : "24";

    // save alarms data
    (*doc)["alarm_urgent_low_enabled"] = settings.alarm_urgent_low_enabled;
    (*doc)["alarm_urgent_low_value"] = settings.alarm_urgent_low_mgdl;
    (*doc)["alarm_urgent_low_snooze_interval"] = settings.alarm_urgent_low_snooze_minutes;
    (*doc)["alarm_urgent_low_silence_interval"] = settings.alarm_urgent_low_silence_interval;
    (*doc)["alarm_low_enabled"] = settings.alarm_low_enabled;
    (*doc)["alarm_low_value"] = settings.alarm_low_mgdl;
    (*doc)["alarm_low_snooze_interval"] = settings.alarm_low_snooze_minutes;
    (*doc)["alarm_low_silence_interval"] = settings.alarm_low_silence_interval;
    (*doc)["alarm_high_enabled"] = settings.alarm_high_enabled;
    (*doc)["alarm_high_value"] = settings.alarm_high_mgdl;
    (*doc)["alarm_high_snooze_interval"] = settings.alarm_high_snooze_minutes;
    (*doc)["alarm_high_silence_interval"] = settings.alarm_high_silence_interval;

    // Additional WiFi
    (*doc)["additional_wifi_enable"] = settings.additional_wifi_enable;
    (*doc)["additional_wifi_type"] = settings.additional_wifi_type;
    (*doc)["additional_ssid"] = settings.additional_wifi_ssid;
    (*doc)["additional_wifi_username"] = settings.additional_wifi_username;
    (*doc)["additional_wifi_password"] = settings.additional_wifi_password;

    // Custom hostname
    (*doc)["custom_hostname_enable"] = settings.custom_hostname_enable;
    (*doc)["custom_hostname"] = settings.custom_hostname;

     // Custom No Data Timer
     (*doc)["custom_nodatatimer_enable"] = settings.custom_nodatatimer_enable;
     (*doc)["custom_nodatatimer"] = settings.custom_nodatatimer;
    
    if (trySaveJsonAsSettings(*doc) == false)
        return false;

    delete doc;

    return true;
}

bool SettingsManager_::trySaveJsonAsSettings(JsonDocument doc) {
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
