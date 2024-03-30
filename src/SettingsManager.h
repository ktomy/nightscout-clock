#ifndef SettingsManager_H
#define SettingsManager_H

#include "enums.h"
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Settings.h>

class SettingsManager_ {
  private:
    SettingsManager_() = default;
    DynamicJsonDocument *readConfigJsonFile();

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