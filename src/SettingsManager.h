#ifndef SettingsManager_H
#define SettingsManager_H

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Settings.h>

#include "enums.h"

class SettingsManager_ {
private:
    SettingsManager_() = default;
    JsonDocument* readConfigJsonFile();

public:
    static SettingsManager_& getInstance();
    void setup();
    bool loadSettingsFromFile();
    bool saveSettingsToFile();
    bool trySaveJsonAsSettings(JsonDocument doc);
    // Returns NULL when the alert window list is usable, otherwise why it is not.
    static const char* validateAlertWindows(JsonVariantConst configured);
    void factoryReset();
    // The repeat intervals the WebUI offers; shared with the save endpoint.
    static bool isValidAlarmRepeatInterval(int intervalSeconds);

    Settings settings;
};

extern SettingsManager_& SettingsManager;

#endif