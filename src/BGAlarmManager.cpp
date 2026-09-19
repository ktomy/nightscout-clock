#include <BGAlarmManager.h>
#include <BGDisplayManager.h>
#include <PeripheryManager.h>
#include <SettingsManager.h>

#include "ServerManager.h"
#include "globals.h"

#define ALARM_REPEAT_INTERVAL_INTENSIVE_SECONDS 2

// The getter for the instantiated singleton instance
BGAlarmManager_& BGAlarmManager_::getInstance() {
    static BGAlarmManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGAlarmManager_& bgAlarmManager = bgAlarmManager.getInstance();

#ifdef DEBUG_ALARMS

int debounceTicks = 0;
int debounceTicks2 = 0;
int debounceTicks3 = 0;
int debounceTicks7 = 0;
int debounceTicks8 = 0;

#endif

void BGAlarmManager_::setup() {
    if (SettingsManager.settings.alarm_urgent_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = 1;
        alarmData.top = SettingsManager.settings.alarm_urgent_low_mgdl;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_urgent_low_snooze_minutes;
        alarmData.alertWindows = SettingsManager.settings.alarm_urgent_low_alert_windows;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = SettingsManager.settings.alarm_urgent_low_melody;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_urgent_low_mgdl + 1;
        alarmData.top = SettingsManager.settings.alarm_low_mgdl - 1;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_low_snooze_minutes;
        alarmData.alertWindows = SettingsManager.settings.alarm_low_alert_windows;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = SettingsManager.settings.alarm_low_melody;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_high_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_high_mgdl;
        alarmData.top = 401;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_high_snooze_minutes;
        alarmData.alertWindows = SettingsManager.settings.alarm_high_alert_windows;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = SettingsManager.settings.alarm_high_melody;
        enabledAlarms.push_back(alarmData);
    }

    if (SettingsManager.settings.alarm_intensive_mode) {
        alarmIntervalSeconds = ALARM_REPEAT_INTERVAL_INTENSIVE_SECONDS;  // repeat every 2 seconds
    } else {
        alarmIntervalSeconds = SettingsManager.settings.alarm_repeat_interval_seconds;
    }
}

// True when this alarm is allowed to sound now: no windows means at any time,
// otherwise only while one of them is open.
static bool isInsideAlertWindow(const std::vector<AlertWindow>& alertWindows) {
    if (alertWindows.empty()) {
        return true;
    }

    tm now;
    if (!ServerManager.tryGetTimezonedTime(now)) {
        // Unknown time (no NTP yet) must never silence an alarm.
        DEBUG_PRINTLN("Alarms: time is not known, ignoring alert windows and alerting anyway");
        return true;
    }

    const int nowMinutes = now.tm_hour * 60 + now.tm_min;
    const int today = now.tm_wday;  // 0 is Sunday, matching AlertWindow::days
    const int yesterday = (today + 6) % 7;

    for (const AlertWindow& window : alertWindows) {
        if (window.startMinutes < window.endMinutes) {
            // Contained in one day, for example Monday to Friday 09:00 - 17:00.
            if (window.days[today] && nowMinutes >= window.startMinutes &&
                nowMinutes < window.endMinutes) {
                return true;
            }
        } else {
            // Runs past midnight, for example every day 18:00 - 08:00. The evening half belongs
            // to today's window and the morning half to the one that opened yesterday.
            if (window.days[today] && nowMinutes >= window.startMinutes) {
                return true;
            }
            if (window.days[yesterday] && nowMinutes < window.endMinutes) {
                return true;
            }
        }
    }

    return false;
}

#ifdef DEBUG_ALARMS
#endif

void BGAlarmManager_::tick() {
    auto glucoseReading = bgDisplayManager.getLastDisplayedGlucoseReading();
    if (glucoseReading == nullptr ||
        glucoseReading->getSecondsAgo() >
            SettingsManager.settings.bg_data_too_old_threshold_minutes * 60) {
        activeAlarm = NULL;

#ifdef DEBUG_ALARMS

        if (debounceTicks % 5000 == 0) {
            DEBUG_PRINTLN("Alarms: no alarms as no glucose readings or readings are old");
        }
        debounceTicks++;
        if (debounceTicks > 5000) {
            debounceTicks = 0;
        }
#endif
        return;
    }

    for (AlarmData& alarmData : enabledAlarms) {
        if (glucoseReading->sgv >= alarmData.bottom && glucoseReading->sgv <= alarmData.top) {
#ifdef DEBUG_ALARMS

            if (debounceTicks2 % 5000 == 0) {
                DEBUG_PRINTLN("Alarms: glucose reading in alarm range");
            }
            debounceTicks2++;
            if (debounceTicks2 > 5000) {
                debounceTicks2 = 0;
            }

#endif

            if (!isInsideAlertWindow(alarmData.alertWindows)) {
#ifdef DEBUG_ALARMS

                if (debounceTicks3 % 5000 == 0) {
                    DEBUG_PRINTLN("Alarms: outside every alert window, staying quiet");
                }
                debounceTicks3++;
                if (debounceTicks3 > 5000) {
                    debounceTicks3 = 0;
                }
#endif
                if (activeAlarm != NULL) {
                    activeAlarm->isSnoozed = false;
                    activeAlarm->lastAlarmTime = 0;
                }
                activeAlarm = NULL;
                return;
            }
            if (activeAlarm == NULL) {
                activeAlarm = &alarmData;
                alarmData.lastAlarmTime = ServerManager.getUtcEpoch();
                alarmData.isSnoozed = false;
                PeripheryManager.playRTTTLString(alarmData.alarmSound);
                DEBUG_PRINTLN("Playing alarm sound (nee alarm occurred)");
            } else {
                if (activeAlarm->isSnoozed) {
                    if (activeAlarm->snoozeTimeMinutes != 0 &&
                        ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                            60 * activeAlarm->snoozeTimeMinutes) {
                        activeAlarm->isSnoozed = false;
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound after snooze");
                    } else {
#ifdef DEBUG_ALARMS

                        if (debounceTicks8 % 5000 == 0) {
                            DEBUG_PRINTLN(
                                "Alarms: snoozed, too early to sound: " +
                                String(ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime) +
                                ", snooze interval: " + String(activeAlarm->snoozeTimeMinutes));
                        }
                        debounceTicks8++;
                        if (debounceTicks8 > 5000) {
                            debounceTicks8 = 0;
                        }
#endif
                    }
                } else {
                    if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                        alarmIntervalSeconds) {
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound (alarm already active, not snoozed)");
                    } else {
#ifdef DEBUG_ALARMS

                        if (debounceTicks7 % 5000 == 0) {
                            DEBUG_PRINTLN(
                                "Alarms: not snoozed, too early to sound: " +
                                String(ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime));
                        }
                        debounceTicks7++;
                        if (debounceTicks7 > 5000) {
                            debounceTicks7 = 0;
                        }
#endif
                    }
                }
            }
            return;
        }
    }
    if (activeAlarm != NULL) {
        activeAlarm->isSnoozed = false;
        activeAlarm->lastAlarmTime = 0;
    }
    activeAlarm = NULL;
}

void BGAlarmManager_::snoozeAlarm() {
    if (activeAlarm != NULL && !activeAlarm->isSnoozed) {
        DEBUG_PRINTLN("Snoozing alarm");
        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_CYAN);
        DisplayManager.printText(0, 6, "Snoozed", TEXT_ALIGNMENT::CENTER, 0);
        DisplayManager.update();
        delay(2000);
        bgDisplayManager.maybeRrefreshScreen(true);
        activeAlarm->isSnoozed = true;
    }
}
