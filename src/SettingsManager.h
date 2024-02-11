#ifndef SettingsManager_H
#define SettingsManager_H

#include "enums.h"
#include <ArduinoJson.h>
#include <IPAddress.h>

class Settings {
  public:
    String ssid;
    String wifi_password;
    String hostname;
    String nightscout_url;
    String nightscout_api_key;
    BG_UNIT bg_units;
    int bg_low_warn_limit;
    int bg_high_warn_limit;
    bool auto_brightness;
    int brightness_level;
    int default_clockface;
    BG_SOURCE bg_source;
    String dexcom_username;
    String dexcom_password;
    DEXCOM_SERVER dexcom_server;
    String tz_libc_value;
};

class SettingsManager_ {
  private:
    SettingsManager_() = default;

  public:
    static SettingsManager_ &getInstance();
    void setup();
    bool loadSettingsFromFile();
    bool saveSettingsToFile();
    bool trySaveJsonAsSettings(DynamicJsonDocument doc);
    void factoryReset();

    Settings settings;
};

extern SettingsManager_ &SettingsManager;

#endif