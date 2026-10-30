#include "SettingsSchedule.h"

#include <Arduino.h>

#include "SettingsTime.h"
#include "globals.h"

// The faces in 1.0.0's order, which was their numeric ID.
static const char* const NUMBERED_FACE_IDS[] = {
    "simple",  "graph",     "graph_and_bg", "big_text", "value_and_diff", "clock",
    "unicorn", "time_only", "simple_dark",  "race_car", "dragon",         "big_text_dark"};
static const int NUMBERED_FACE_COUNT = sizeof(NUMBERED_FACE_IDS) / sizeof(NUMBERED_FACE_IDS[0]);

String readFaceId(JsonVariantConst value) {
    if (value.is<int>()) {
        const int number = value.as<int>();
        if (number >= 0 && number < NUMBERED_FACE_COUNT) {
            return NUMBERED_FACE_IDS[number];
        }
    }
    // Any other value is kept, and an unknown one is ignored like any unknown face.
    return value.as<String>();
}

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
        entry.face = readFaceId(item["face"]);
        entry.brightness = item["brightness"] | 0;

        const bool brightnessIsKnown = (entry.brightness >= 1 && entry.brightness <= 10) ||
                                       entry.brightness == 100 || entry.brightness == 101;
        if (entry.startMinutes < 0 || !brightnessIsKnown) {
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
