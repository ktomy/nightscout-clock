#ifndef BGAlarmManager_h
#define BGAlarmManager_h

#include <Arduino.h>
#include <SettingsAlarm.h>

#include <vector>

struct AlarmData {
    int bottom;
    int top;
    unsigned long lastAlarmTime;
    int snoozeTimeMinutes;
    std::vector<AlertWindow> alertWindows;
    String alarmSound;
    bool isSnoozed;
};

class BGAlarmManager_ {
private:
    BGAlarmManager_() = default;
    std::vector<AlarmData> enabledAlarms;
    AlarmData* activeAlarm;
    int alarmIntervalSeconds;

public:
    static BGAlarmManager_& getInstance();
    void setup();
    void tick();
    void snoozeAlarm();
    // True while a fresh reading is in an enabled alarm's range and alert window, snoozed or not.
    bool isAlarmActive() const;
};

extern BGAlarmManager_& bgAlarmManager;
#endif
