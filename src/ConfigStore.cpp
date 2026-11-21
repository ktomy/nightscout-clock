#include "ConfigStore.h"

#include <Preferences.h>
#include <nvs.h>
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
    // The NVS calls themselves, not Preferences, which reports any error as "not there": only a
    // namespace or key that does not exist means nothing is saved yet.
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ReadResult::NOTHING_SAVED;
    }
    if (err != ESP_OK) {
        DEBUG_PRINTF("Settings storage could not be opened: %s", esp_err_to_name(err));
        return ReadResult::FAILED;
    }
    size_t size = 0;
    err = nvs_get_blob(handle, KEY, nullptr, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND || (err == ESP_OK && size == 0)) {
        nvs_close(handle);
        return ReadResult::NOTHING_SAVED;
    }
    // One more byte for the terminator, which String(buffer, length) copies.
    std::unique_ptr<char[]> buffer(err == ESP_OK ? new (std::nothrow) char[size + 1] : nullptr);
    if (buffer) {
        err = nvs_get_blob(handle, KEY, buffer.get(), &size);
    }
    nvs_close(handle);
    if (!buffer || err != ESP_OK) {
        DEBUG_PRINTF("The %u-byte settings could not be read: %s", size, esp_err_to_name(err));
        return ReadResult::FAILED;
    }
    buffer[size] = '\0';
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
