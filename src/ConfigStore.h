#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Where the settings live: the config JSON as one NVS entry in the "config" partition
// (partitions.csv), and the factory settings built into the firmware. Firmware updates and USB
// installs without erasing leave the entry as it is. The settings page's preview has its own,
// in-memory version (web/emulator/platform.cpp).
namespace ConfigStore {
void setup();
enum class ReadResult { OK, NOTHING_SAVED, FAILED };
// The saved config JSON. FAILED, such as for lack of memory, does not mean nothing is saved.
ReadResult read(String& json);
bool write(const String& json);
// data/config_initial.json, and in the Wokwi build scripts/wokwi_config.json laid over it.
void loadFactorySettings(JsonDocument& doc);
}  // namespace ConfigStore

#endif
