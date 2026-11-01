#ifndef SettingsAlarm_h
#define SettingsAlarm_h

#include <ArduinoJson.h>

#include <vector>

#include "SettingsAlertWindow.h"

std::vector<AlertWindow> readAlertWindows(JsonVariantConst configured);
void writeAlertWindows(
    JsonDocument& doc, const char* windowsKey, const std::vector<AlertWindow>& windows);

#endif
