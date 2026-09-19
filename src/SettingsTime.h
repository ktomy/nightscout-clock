#ifndef SettingsTime_h
#define SettingsTime_h

#include <Arduino.h>

// "HH:MM" as minutes since midnight, or -1 when it is not a readable time of day.
int parseTimeOfDayMinutes(const String& value);
String minutesAsTimeOfDay(int minutes);

#endif
