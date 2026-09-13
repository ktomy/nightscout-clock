#ifndef SettingsAlarm_h
#define SettingsAlarm_h

#include <ArduinoJson.h>

#include <vector>

// Selected days are the days the window starts on, including windows that cross midnight.
struct AlertWindow {
    bool days[7] = {};     // Sunday through Saturday
    int startMinutes = 0;  // minutes since midnight, 0-1439
    int endMinutes = 0;
};

std::vector<AlertWindow> readAlertWindows(JsonVariantConst configured);
void writeAlertWindows(
    JsonDocument& doc, const char* windowsKey, const std::vector<AlertWindow>& windows);

#endif
