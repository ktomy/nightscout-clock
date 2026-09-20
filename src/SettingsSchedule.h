#ifndef SettingsSchedule_h
#define SettingsSchedule_h

#include <ArduinoJson.h>

#include <vector>

// From this time of day, show this face at this brightness. The brightness is the Web UI's
// value: 1-10 for a manual level, 100 and 101 for the two automatic modes.
struct FaceScheduleEntry {
    int startMinutes = 0;  // minutes since midnight, 0-1439
    int face = 0;
    int brightness = 100;
};

#define FACE_SCHEDULE_MAX_ENTRIES 8

std::vector<FaceScheduleEntry> readFaceSchedule(JsonVariantConst configured);
void writeFaceSchedule(
    JsonDocument& doc, const char* key, const std::vector<FaceScheduleEntry>& entries);

#endif
