#include <BGAlarmManager.h>
#include <SettingsManager.h>
#include <BGDisplayManager.h>
#include "globals.h"
#include "ServerManager.h"
#include <PeripheryManager.h>

// The getter for the instantiated singleton instance
BGAlarmManager_ &BGAlarmManager_::getInstance() {
    static BGAlarmManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGAlarmManager_ &bgAlarmManager = bgAlarmManager.getInstance();

void BGAlarmManager_::setup() {
    if (SettingsManager.settings.alarm_urgent_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = 1;
        alarmData.top = SettingsManager.settings.alarm_urgent_low_mgdl;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_urgent_low_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_urgent_low_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = sound_urgent_low;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_urgent_low_mgdl + 1;
        alarmData.top = SettingsManager.settings.alarm_low_mgdl - 1;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_low_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_low_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = sound_low;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_high_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_high_mgdl;
        alarmData.top = 401;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_high_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_high_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = sound_high;
        enabledAlarms.push_back(alarmData);
    }
}

bool isInSilentInterval(String silenceInterval) {
    // TODO: Implement this function
    return false;
}

void BGAlarmManager_::tick() {
    auto glucoseReading = bgDisplayManager.getLastDisplayedGlucoseReading();
    if (glucoseReading == nullptr || glucoseReading->getSecondsAgo() > BG_DATA_OLD_OFFSET_MINUTES * 60) {
        activeAlarm = NULL;
        return;
    }

    for (AlarmData &alarmData : enabledAlarms) {
        if (glucoseReading->sgv >= alarmData.bottom && glucoseReading->sgv <= alarmData.top) {
            if (isInSilentInterval(alarmData.silenceInterval)) {
                if (activeAlarm != NULL) {
                    activeAlarm->isSnoozed = false;
                }
                activeAlarm = NULL;
                return;
            }
            if (activeAlarm == NULL) {
                activeAlarm = &alarmData;
                alarmData.lastAlarmTime = ServerManager.getUtcEpoch();
                PeripheryManager.playRTTTLString(alarmData.alarmSound);
            } else {
                if (activeAlarm->isSnoozed) {
                    if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime > 60 * activeAlarm->snoozeTimeMinutes) {
                        activeAlarm->isSnoozed = false;
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                    }
                } else {
                    if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime > 60 * 5) {
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
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
    if (activeAlarm != NULL) {
        DEBUG_PRINTLN("Snoozing alarm");
        activeAlarm->isSnoozed = true;
        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
    }
}
