#include <BGAlarmManager.h>
#include <SettingsManager.h>
#include <BGDisplayManager.h>
#include "globals.h"
#include "ServerManager.h"
#include <PeripheryManager.h>

#define ALARM_REPEAT_INTERVAL_SECONDS 300

// The getter for the instantiated singleton instance
BGAlarmManager_ &BGAlarmManager_::getInstance() {
    static BGAlarmManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGAlarmManager_ &bgAlarmManager = bgAlarmManager.getInstance();

#ifdef DEBUG_ALARMS

int debounceTicks = 0;
int debounceTicks2 = 0;
int debounceTicks3 = 0;
int debounceTicks4 = 0;
int debounceTicks5 = 0;
int debounceTicks6 = 0;
int debounceTicks7 = 0;

#endif

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
    if (silenceInterval == "" || silenceInterval == "0") {
#ifdef DEBUG_ALARMS

        if (debounceTicks3 % 5000 == 0) {
            DEBUG_PRINTLN("Alarms: no silence interval defined");
        }
        debounceTicks3++;
        if (debounceTicks3 > 5000) {
            debounceTicks3 = 0;
        }

#endif        
        return false;
    }

    auto time = ServerManager.getTimezonedTime();

    if (silenceInterval == "22_8" && (time.tm_hour >= 22 || time.tm_hour < 8)) {
#ifdef DEBUG_ALARMS
    
            if (debounceTicks4 % 5000 == 0) {
                DEBUG_PRINTLN("Alarms: silence interval 22_8 active");
            }
            debounceTicks4++;
            if (debounceTicks4 > 5000) {
                debounceTicks4 = 0;
            }   
#endif
        return true;
    }

    if (silenceInterval == "8_22" && (time.tm_hour >= 8 && time.tm_hour < 22)) {
#ifdef DEBUG_ALARMS
        
                if (debounceTicks5 % 5000 == 0) {
                    DEBUG_PRINTLN("Alarms: silence interval 8_22 active");
                }
                debounceTicks5++;
                if (debounceTicks5 > 5000) {
                    debounceTicks5 = 0;
                }
#endif        
        return true;
    }

#ifdef DEBUG_ALARMS
        
    if (debounceTicks6 % 5000 == 0) {
        DEBUG_PRINTLN("Alarms: silence interval exists but not active");
    }
    debounceTicks6++;
    if (debounceTicks6 > 5000) {
        debounceTicks6 = 0;
    }
#endif
    return false;
}

#ifdef DEBUG_ALARMS
#endif

void BGAlarmManager_::tick() {
    auto glucoseReading = bgDisplayManager.getLastDisplayedGlucoseReading();
    if (glucoseReading == nullptr || glucoseReading->getSecondsAgo() > BG_DATA_OLD_OFFSET_MINUTES * 60) {
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

    for (AlarmData &alarmData : enabledAlarms) {
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

            if (isInSilentInterval(alarmData.silenceInterval)) {
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
                    if (activeAlarm->snoozeTimeMinutes != 0 && ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime > 60 * activeAlarm->snoozeTimeMinutes) {
                        activeAlarm->isSnoozed = false;
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound after snooze");
                    }
                    else {
#ifdef DEBUG_ALARMS

                        if (debounceTicks8 % 5000 == 0) {
                            DEBUG_PRINTLN("Alarms: snoozed, too early to sound: " + String(ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime) + ", snooze interval: " + String(activeAlarm->snoozeTimeMinutes));
                        }
                        debounceTicks8++;
                        if (debounceTicks8 > 5000) {
                            debounceTicks8 = 0;
                        }
#endif
                    }
                } else {
                    if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime > ALARM_REPEAT_INTERVAL_SECONDS) {
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound (alarm already active, not snoozed)");
                    }
                    else {
#ifdef DEBUG_ALARMS

                        if (debounceTicks7 % 5000 == 0) {
                            DEBUG_PRINTLN("Alarms: not snoozed, too early to sound: " + String(ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime));
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
    if (activeAlarm != NULL) {
        DEBUG_PRINTLN("Snoozing alarm");
        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_CYAN);
        DisplayManager.printText(0, 6, "Snoozed", TEXT_ALIGNMENT::CENTER, 0);
        delay(2000);
        bgDisplayManager.maybeRrefreshScreen(true);
        activeAlarm->isSnoozed = true;
    }
}
