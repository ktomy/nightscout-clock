#include "ConfigStore.h"

#include <Preferences.h>
#include <nvs_flash.h>

#include <memory>
#include <new>

#include "EmbeddedFiles.h"
#include "SettingsOverlay.h"
#include "globals.h"

DECLARE_EMBEDDED_FILE(configInitial, data_config_initial_json)
#ifdef WOKWI
DECLARE_EMBEDDED_FILE(wokwiConfig, scripts_wokwi_config_json)
#endif

namespace {
const char* PARTITION = "config";
const char* NAMESPACE = "settings";
// A blob, not a string: NVS strings stop at 4000 bytes, and the config is already 3 KB. A blob can
// take most of the 64 KB partition, and a rewrite keeps the old copy until the new one is complete.
const char* KEY = "json";

void deserializeEmbedded(JsonDocument& doc, EmbeddedFile file) {
    DeserializationError error =
        deserializeJson(doc, reinterpret_cast<const char*>(file.data), file.size);
    if (error) {
        DEBUG_PRINTF("Built-in settings could not be read: %s", error.c_str());
    }
}
}  // namespace

void ConfigStore::setup() {
    esp_err_t err = nvs_flash_init_partition(PARTITION);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Data NVS cannot use, such as what a firmware with another partition table left there.
        DEBUG_PRINTF("Settings storage unreadable (%s), erasing it", esp_err_to_name(err));
        nvs_flash_erase_partition(PARTITION);
        err = nvs_flash_init_partition(PARTITION);
    }
    if (err != ESP_OK) {
        DEBUG_PRINTF("Settings storage unavailable: %s", esp_err_to_name(err));
    }
}

ConfigStore::ReadResult ConfigStore::read(String& json) {
    Preferences prefs;
    // Opening for reading fails until something has been saved.
    if (!prefs.begin(NAMESPACE, true, PARTITION)) {
        return ReadResult::NOTHING_SAVED;
    }
    size_t size = prefs.isKey(KEY) ? prefs.getBytesLength(KEY) : 0;
    if (size == 0) {
        prefs.end();
        return ReadResult::NOTHING_SAVED;
    }
    std::unique_ptr<char[]> buffer(new (std::nothrow) char[size]);
    if (!buffer) {
        DEBUG_PRINTF("No memory to read the %u-byte settings", size);
        prefs.end();
        return ReadResult::FAILED;
    }
    size_t got = prefs.getBytes(KEY, buffer.get(), size);
    prefs.end();
    if (got != size) {
        return ReadResult::FAILED;
    }
    json = String(buffer.get(), size);
    return json.length() == size ? ReadResult::OK : ReadResult::FAILED;
}

bool ConfigStore::write(const String& json) {
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false, PARTITION)) {
        return false;
    }
    size_t written = prefs.putBytes(KEY, json.c_str(), json.length());
    prefs.end();
    return written == json.length();
}

void ConfigStore::loadFactorySettings(JsonDocument& doc) {
    deserializeEmbedded(doc, embeddedFile(configInitial_start, configInitial_end));
#ifdef WOKWI
    JsonDocument wokwi;
    deserializeEmbedded(wokwi, embeddedFile(wokwiConfig_start, wokwiConfig_end));
    overlaySettings(doc.as<JsonVariant>(), wokwi.as<JsonVariantConst>());
#endif
}
