#ifndef SettingsManager_H
#define SettingsManager_H

#include "enums.h"
#include <IPAddress.h>
#include <ArduinoJson.h>

class Settings
{
public:
    String ssid;
    String password;
    String hostname;
    String nsUrl;
    String nsApiKey;
    BG_UNIT bgUnit;
    int bgLow;
    int bgHigh;
};

class SettingsManager_
{
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