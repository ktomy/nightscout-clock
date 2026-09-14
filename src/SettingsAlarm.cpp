#include "SettingsAlarm.h"

#include <Arduino.h>

#include "globals.h"

int parseTimeOfDayMinutes(const String& value) {
    int colon = value.indexOf(':');
    if (colon < 1 || (int)value.length() - colon != 3) {
        return -1;
    }
    for (unsigned int i = 0; i < value.length(); i++) {
        if ((int)i != colon && !isDigit(value[i])) {
            return -1;
        }
    }

    int hours = value.substring(0, colon).toInt();
    int minutes = value.substring(colon + 1).toInt();
    if (hours > 23 || minutes > 59) {
        return -1;
    }
    return hours * 60 + minutes;
}

String minutesAsTimeOfDay(int minutes) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
    return String(buffer);
}

namespace {
// Days use tm_wday numbering: "12345" means Monday to Friday.
bool parseAlertWindowDays(const String& value, AlertWindow& window) {
    if (value.length() == 0 || value.length() > 7) {
        return false;
    }

    for (unsigned int i = 0; i < value.length(); i++) {
        char day = value[i];
        if (day < '0' || day > '6') {
            return false;
        }

        if (window.days[day - '0']) {
            return false;
        }
        window.days[day - '0'] = true;
    }

    return true;
}

String alertWindowDaysAsString(const AlertWindow& window) {
    String value = "";
    for (int day = 0; day < 7; day++) {
        if (window.days[day]) {
            value += (char)('0' + day);
        }
    }
    return value;
}

}  // namespace

std::vector<AlertWindow> readAlertWindows(JsonVariantConst configured) {
    std::vector<AlertWindow> windows;
    if (!configured.is<JsonArrayConst>()) {
        return windows;
    }

    for (JsonVariantConst entry : configured.as<JsonArrayConst>()) {
        if (!entry.is<JsonObjectConst>()) {
            continue;
        }

        AlertWindow window;
        const bool daysAreReadable = parseAlertWindowDays(entry["days"].as<String>(), window);
        window.startMinutes = parseTimeOfDayMinutes(entry["from"].as<String>());
        window.endMinutes = parseTimeOfDayMinutes(entry["to"].as<String>());

        // Skip incomplete or zero-length windows when loading settings.
        if (!daysAreReadable || window.startMinutes < 0 || window.endMinutes < 0 ||
            window.startMinutes == window.endMinutes) {
            DEBUG_PRINTLN("Ignoring an alert window that could never open");
            continue;
        }

        windows.push_back(window);
    }

    return windows;
}

void writeAlertWindows(
    JsonDocument& doc, const char* windowsKey, const std::vector<AlertWindow>& windows) {
    doc.remove(windowsKey);

    JsonArray configured = doc[windowsKey].to<JsonArray>();
    for (const AlertWindow& window : windows) {
        JsonObject entry = configured.add<JsonObject>();
        entry["days"] = alertWindowDaysAsString(window);
        entry["from"] = minutesAsTimeOfDay(window.startMinutes);
        entry["to"] = minutesAsTimeOfDay(window.endMinutes);
    }
}
