#ifndef SettingsManager_H
#define SettingsManager_H

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Settings.h>

#include <atomic>
#include <mutex>

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
    void factoryReset();
    // The repeat intervals the WebUI offers; shared with the save endpoint.
    static bool isValidAlarmRepeatInterval(int intervalSeconds);
    // Parses a colon- or dash-separated MAC string (e.g. AA:BB:CC:DD:EE:FF)
    // into 6 bytes. Shared with the save endpoint for validation.
    static bool parseCustomMac(const String& macStr, uint8_t* macBytes);
    // Set by the web server after a save; the main loop loads and applies it.
    std::atomic<bool> reloadRequested{false};
    // Held to load or save settings, and by the web server (its own task) to read them.
    std::recursive_mutex mutex;

    Settings settings;
};

extern SettingsManager_& SettingsManager;

#endif