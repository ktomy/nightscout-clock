#ifndef SettingsAlertWindow_h
#define SettingsAlertWindow_h

#include <vector>

// This header stays free of Arduino dependencies so that test/ can compile it on the host.

// Selected days are the days the window starts on, including windows that cross midnight.
struct AlertWindow {
    bool days[7] = {};     // Sunday through Saturday
    int startMinutes = 0;  // minutes since midnight, 0-1439
    int endMinutes = 0;
};

// Whether a time falls inside any of the windows. Weekday uses tm_wday numbering (0 is Sunday).
inline bool isInsideAlertWindow(
    const std::vector<AlertWindow>& alertWindows, int nowMinutes, int weekday) {
    const int yesterday = (weekday + 6) % 7;

    for (const AlertWindow& window : alertWindows) {
        if (window.startMinutes < window.endMinutes) {
            // Contained in one day, for example Monday to Friday 09:00 - 17:00.
            if (window.days[weekday] && nowMinutes >= window.startMinutes &&
                nowMinutes < window.endMinutes) {
                return true;
            }
        } else {
            // Runs past midnight, for example every day 18:00 - 08:00. The evening half belongs
            // to today's window and the morning half to the one that opened yesterday.
            if (window.days[weekday] && nowMinutes >= window.startMinutes) {
                return true;
            }
            if (window.days[yesterday] && nowMinutes < window.endMinutes) {
                return true;
            }
        }
    }

    return false;
}

#endif
