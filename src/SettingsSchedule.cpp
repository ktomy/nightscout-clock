#include "SettingsSchedule.h"

#include <Arduino.h>

#include "SettingsAlarm.h"
#include "globals.h"

std::vector<FaceScheduleEntry> readFaceSchedule(JsonVariantConst configured) {
    std::vector<FaceScheduleEntry> entries;
    if (!configured.is<JsonArrayConst>()) {
        return entries;
    }

    for (JsonVariantConst item : configured.as<JsonArrayConst>()) {
        if (entries.size() >= FACE_SCHEDULE_MAX_ENTRIES) {
            break;
        }
        if (!item.is<JsonObjectConst>()) {
            continue;
        }

        FaceScheduleEntry entry;
        entry.startMinutes = parseTimeOfDayMinutes(item["time"].as<String>());
        entry.face = item["face"] | -1;
        entry.brightness = item["brightness"] | 0;

        const bool brightnessIsKnown = (entry.brightness >= 1 && entry.brightness <= 10) ||
                                       entry.brightness == 100 || entry.brightness == 101;
        if (entry.startMinutes < 0 || entry.face < 0 || entry.face >= CLOCK_FACE_COUNT ||
            !brightnessIsKnown) {
            DEBUG_PRINTLN("Ignoring a schedule row that cannot be applied");
            continue;
        }

        entries.push_back(entry);
    }

    return entries;
}

void writeFaceSchedule(
    JsonDocument& doc, const char* key, const std::vector<FaceScheduleEntry>& entries) {
    doc.remove(key);

    JsonArray configured = doc[key].to<JsonArray>();
    for (const FaceScheduleEntry& entry : entries) {
        JsonObject item = configured.add<JsonObject>();
        item["time"] = minutesAsTimeOfDay(entry.startMinutes);
        item["face"] = entry.face;
        item["brightness"] = entry.brightness;
    }
}
