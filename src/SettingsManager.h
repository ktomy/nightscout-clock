#ifndef SettingsManager_H
#define SettingsManager_H

#include "enums.h"
#include <ArduinoJson.h>
#include <IPAddress.h>

class Settings {
  public:
    String ssid;
    String password;
    String hostname;
    String nsUrl;
    String nsApiKey;
    BG_UNIT bgUnit;
    int bgLow;
    int bgHigh;
    bool auto_brightness;
    int brightness_level;
    int default_clockface;
    BG_SOURCE bg_source;
    String dexom_username;
    String dexcom_password;
    String dexcom_server;
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