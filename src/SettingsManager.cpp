#include "SettingsManager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "globals.h"

namespace {
bool isValidFaceCycleInterval(int intervalSeconds) {
    return intervalSeconds == 10 || intervalSeconds == 30 || intervalSeconds == 60 ||
           intervalSeconds == 120 || intervalSeconds == 180 || intervalSeconds == 300;
}

// "HH:MM" as minutes since midnight, or -1 when it is not a readable time of day.
int parseTimeOfDayMinutes(const String& value) {
    int colon = value.indexOf(':');
    if (colon < 1 || (int)value.length() - colon != 3) {
        return -1;
    }
    for (unsigned int i = 0; i < value.length(); i++) {
        if ((int)i != colon && !isDigit(value[i])) {
            return -1;
        }
    }

    int hours = value.substring(0, colon).toInt();
    int minutes = value.substring(colon + 1).toInt();
    if (hours > 23 || minutes > 59) {
        return -1;
    }
    return hours * 60 + minutes;
}

String minutesAsTimeOfDay(int minutes) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
    return String(buffer);
}

// Days are tm_wday digits ("12345" = Monday to Friday). Anything unreadable is refused,
// not skipped, because a partial day list would silence an alarm on the wrong days.
bool parseAlertWindowDays(const String& value, uint8_t& days) {
    days = 0;
    if (value.length() == 0 || value.length() > 7) {
        return false;
    }

    for (unsigned int i = 0; i < value.length(); i++) {
        char day = value[i];
        if (day < '0' || day > '6') {
            return false;
        }

        uint8_t dayBit = (uint8_t)(1 << (day - '0'));
        if (days & dayBit) {
            return false;
        }
        days |= dayBit;
    }

    return true;
}

String alertWindowDaysAsString(uint8_t days) {
    String value = "";
    for (int day = 0; day < 7; day++) {
        if (days & (1 << day)) {
            value += (char)('0' + day);
        }
    }
    return value;
}

std::vector<AlertWindow> readAlertWindows(JsonVariantConst configured) {
    std::vector<AlertWindow> windows;
    if (!configured.is<JsonArrayConst>()) {
        return windows;
    }

    for (JsonVariantConst entry : configured.as<JsonArrayConst>()) {
        if (!entry.is<JsonObjectConst>()) {
            continue;
        }

        AlertWindow window;
        const bool daysAreReadable = parseAlertWindowDays(entry["days"].as<String>(), window.days);
        window.startMinutes = parseTimeOfDayMinutes(entry["from"].as<String>());
        window.endMinutes = parseTimeOfDayMinutes(entry["to"].as<String>());

        // A window that cannot be read can never open; drop it rather than let it silence an alarm.
        if (!daysAreReadable || window.startMinutes < 0 || window.endMinutes < 0 ||
            window.startMinutes == window.endMinutes) {
            DEBUG_PRINTLN("Ignoring an alert window that could never open");
            continue;
        }

        windows.push_back(window);
    }

    return windows;
}

// Translate a pre-window silence interval into the equivalent alert window (lossless).
std::vector<AlertWindow> alertWindowsFromSilenceInterval(const String& silenceInterval) {
    std::vector<AlertWindow> windows;
    AlertWindow window;
    window.days = 0x7F;  // every day

    if (silenceInterval == "22_8") {
        window.startMinutes = 8 * 60;  // silent 22:00 - 08:00, so alerting 08:00 - 22:00
        window.endMinutes = 22 * 60;
    } else if (silenceInterval == "8_22") {
        window.startMinutes = 22 * 60;  // silent 08:00 - 22:00, so alerting 22:00 - 08:00
        window.endMinutes = 8 * 60;
    } else {
        return windows;  // "0", empty or unrecognised: alert at any time
    }

    windows.push_back(window);
    return windows;
}

std::vector<AlertWindow> loadAlertWindows(JsonDocument& doc, const char* windowsKey,
                                          const char* legacySilenceKey) {
    if (doc[windowsKey].is<JsonArrayConst>()) {
        return readAlertWindows(doc[windowsKey]);
    }

    DEBUG_PRINTF("No alert windows for %s, migrating the silence interval instead\n", windowsKey);
    return alertWindowsFromSilenceInterval(doc[legacySilenceKey].as<String>());
}

void writeAlertWindows(JsonDocument& doc, const char* windowsKey, const char* legacySilenceKey,
                       const std::vector<AlertWindow>& windows) {
    doc.remove(windowsKey);
    // Drop the old key once windows are written, or the migration would re-run on the next load.
    doc.remove(legacySilenceKey);

    JsonArray configured = doc[windowsKey].to<JsonArray>();
    for (const AlertWindow& window : windows) {
        JsonObject entry = configured.add<JsonObject>();
        entry["days"] = alertWindowDaysAsString(window.days);
        entry["from"] = minutesAsTimeOfDay(window.startMinutes);
        entry["to"] = minutesAsTimeOfDay(window.endMinutes);
    }
}
}  // namespace

// The getter for the instantiated singleton instance
SettingsManager_& SettingsManager_::getInstance() {
    static SettingsManager_ instance;
    return instance;
}

// Initialize the global shared instance
SettingsManager_& SettingsManager = SettingsManager.getInstance();

void SettingsManager_::setup() { LittleFS.begin(); }

const char* SettingsManager_::validateAlertWindows(JsonVariantConst configured) {
    if (configured.isNull()) {
        return NULL;
    }
    if (!configured.is<JsonArrayConst>()) {
        return "Alert windows must be an array";
    }

    for (JsonVariantConst entry : configured.as<JsonArrayConst>()) {
        if (!entry.is<JsonObjectConst>()) {
            return "Every alert window must be an object";
        }
        uint8_t days;
        if (!parseAlertWindowDays(entry["days"].as<String>(), days)) {
            return "Alert window days must be digits from 0 to 6 where 0 is Sunday, each used "
                   "at most once";
        }

        int startMinutes = parseTimeOfDayMinutes(entry["from"].as<String>());
        int endMinutes = parseTimeOfDayMinutes(entry["to"].as<String>());
        if (startMinutes < 0 || endMinutes < 0) {
            return "Alert window times must be written as HH:MM";
        }
        if (startMinutes == endMinutes) {
            return "An alert window cannot start and end at the same time";
        }
    }

    return NULL;
}

bool copyFile(const char* srcPath, const char* destPath) {
    File srcFile = LittleFS.open(srcPath, "r");
    if (!srcFile) {
        DEBUG_PRINTLN("Failed to open source file");
        return false;
    }

    File destFile = LittleFS.open(destPath, "w");
    if (!destFile) {
        DEBUG_PRINTLN("Failed to open destination file");
        srcFile.close();
        return false;
    }

    while (srcFile.available()) {
        char data = srcFile.read();
        destFile.write(data);
    }

    srcFile.close();
    destFile.close();

    DEBUG_PRINTLN("File copied successfully");
    return true;
}

void SettingsManager_::factoryReset() {
    copyFile(CONFIG_JSON_FACTORY, CONFIG_JSON);
    LittleFS.end();
    ESP.restart();
}

JsonDocument* SettingsManager_::readConfigJsonFile() {
    JsonDocument* doc;
    if (LittleFS.exists(CONFIG_JSON)) {
        File file = LittleFS.open(CONFIG_JSON);
        if (!file || file.isDirectory()) {
            DEBUG_PRINTLN("Failed to open config file for reading");
            return NULL;
        }

        doc = new JsonDocument();
        DeserializationError error = deserializeJson(*doc, file);
        if (error) {
            DEBUG_PRINTF(
                "Deserialization error. File size: %d, requested memory: %d. Error: %s\n", file.size(),
                (int)(file.size() * 2), error.c_str());
            file.close();
            return NULL;
        }
        return doc;
    } else {
        DEBUG_PRINTLN("Cannot read configuration file");
        factoryReset();
        return NULL;
    }
}

bool SettingsManager_::isValidAlarmRepeatInterval(int intervalSeconds) {
    return intervalSeconds == 60 || intervalSeconds == 120 || intervalSeconds == 300;
}

bool SettingsManager_::loadSettingsFromFile() {
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    settings.ssid = (*doc)["ssid"].as<String>();
    settings.wifi_password = (*doc)["password"].as<String>();

    settings.bg_low_warn_limit = (*doc)["low_mgdl"].as<int>();
    settings.bg_high_warn_limit = (*doc)["high_mgdl"].as<int>();
    settings.bg_low_urgent_limit = (*doc)["low_urgent_mgdl"].as<int>();
    settings.bg_high_urgent_limit = (*doc)["high_urgent_mgdl"].as<int>();
    settings.bg_units = (*doc)["units"].as<String>() == "mmol" ? BG_UNIT::MMOLL : BG_UNIT::MGDL;

    String brightness_mode = (*doc)["brightness_mode"].as<String>();
    if (brightness_mode == "manual") {
        settings.brightness_mode = BRIGHTNES_MODE::MANUAL;
    } else if (brightness_mode == "auto_linear") {
        settings.brightness_mode = BRIGHTNES_MODE::AUTO_LINEAR;
    } else if (brightness_mode == "auto_dimmed") {
        settings.brightness_mode = BRIGHTNES_MODE::AUTO_DIMMED;
    } else {
        DEBUG_PRINTLN(
            "Unknown brightness mode in config: " + brightness_mode + ", defaulting to AUTO_LINEAR");
        settings.brightness_mode = BRIGHTNES_MODE::AUTO_LINEAR;
    }

    settings.brightness_level = (*doc)["brightness_level"].as<int>() - 1;
    settings.default_clockface = (*doc)["default_face"].as<int>();

    settings.face_cycle_enabled = (*doc)["face_cycle_enabled"] | false;
    settings.face_cycle_interval_seconds = (*doc)["face_cycle_interval_seconds"] | 60;
    if (!isValidFaceCycleInterval(settings.face_cycle_interval_seconds)) {
        DEBUG_PRINTLN("Invalid face cycle interval in config, defaulting to 60 seconds");
        settings.face_cycle_interval_seconds = 60;
    }

    settings.face_cycle_faces.clear();
    bool faceAlreadyAdded[CLOCK_FACE_COUNT] = {};
    if ((*doc)["face_cycle_faces"].is<JsonArray>()) {
        for (JsonVariant face : (*doc)["face_cycle_faces"].as<JsonArray>()) {
            if (!face.is<int>()) {
                continue;
            }

            int faceId = face.as<int>();
            if (faceId >= 0 && faceId < CLOCK_FACE_COUNT && !faceAlreadyAdded[faceId]) {
                settings.face_cycle_faces.push_back(faceId);
                faceAlreadyAdded[faceId] = true;
            }
        }
    }
    if (settings.face_cycle_faces.empty()) {
        int fallbackFace = settings.default_clockface >= 0 && settings.default_clockface < CLOCK_FACE_COUNT
                               ? settings.default_clockface
                               : 0;
        settings.face_cycle_faces.push_back(fallbackFace);
    }
    if (settings.face_cycle_enabled && settings.face_cycle_faces.size() < 2) {
        DEBUG_PRINTLN("Too few valid faces in config, disabling face cycling");
        settings.face_cycle_enabled = false;
    }

    String data_source = (*doc)["data_source"].as<String>();
    if (data_source == "nightscout") {
        settings.bg_source = BG_SOURCE::NIGHTSCOUT;
    } else if (data_source == "dexcom") {
        settings.bg_source = BG_SOURCE::DEXCOM;
    } else if (data_source == "medtronic") {
        settings.bg_source = BG_SOURCE::MEDTRONIC;
    } else if (data_source == "api") {
        settings.bg_source = BG_SOURCE::API;
    } else if (data_source == "librelinkup") {
        settings.bg_source = BG_SOURCE::LIBRELINKUP;
    } else if (data_source == "medtrum") {
        settings.bg_source = BG_SOURCE::MEDTRUM;
    } else {
        settings.bg_source = BG_SOURCE::NO_SOURCE;
    }
    settings.medtrum_email = (*doc)["medtrum_email"].as<String>();
    settings.medtrum_password = (*doc)["medtrum_password"].as<String>();
    settings.dexcom_username = (*doc)["dexcom_username"].as<String>();
    settings.dexcom_password = (*doc)["dexcom_password"].as<String>();
    String dexcomServerStr = (*doc)["dexcom_server"].as<String>();
    if (dexcomServerStr == "us") {
        settings.dexcom_server = DEXCOM_SERVER::US;
    } else if (dexcomServerStr == "ous") {
        settings.dexcom_server = DEXCOM_SERVER::NON_US;
    } else if (dexcomServerStr == "jp") {
        settings.dexcom_server = DEXCOM_SERVER::JAPAN;
    } else {
        DEBUG_PRINTLN("Unknown Dexcom server in config, defaulting to NON_US");
        settings.dexcom_server = DEXCOM_SERVER::NON_US;
    }

    settings.librelinkup_email = (*doc)["librelinkup_email"].as<String>();
    settings.librelinkup_password = (*doc)["librelinkup_password"].as<String>();
    settings.librelinkup_region = (*doc)["librelinkup_region"].as<String>();
    settings.librelinkup_patient_id = (*doc)["librelinkup_patient_id"].as<String>();

    settings.nightscout_url = (*doc)["nightscout_url"].as<String>();
    settings.nightscout_api_key = (*doc)["api_secret"].as<String>();
    settings.nightscout_simplified_api = (*doc)["nightscout_simplified_api"].as<bool>();

    settings.tz_libc_value = (*doc)["tz_libc"].as<String>();
    settings.time_format =
        (*doc)["time_format"].as<String>() == "12" ? TIME_FORMAT::HOURS_12 : TIME_FORMAT::HOURS_24;

    // read alarms data
    settings.alarm_urgent_low_enabled = (*doc)["alarm_urgent_low_enabled"].as<bool>();
    settings.alarm_urgent_low_mgdl = (*doc)["alarm_urgent_low_value"].as<int>();
    settings.alarm_urgent_low_snooze_minutes = (*doc)["alarm_urgent_low_snooze_interval"].as<int>();
    settings.alarm_urgent_low_alert_windows =
        loadAlertWindows(*doc, "alarm_urgent_low_alert_windows", "alarm_urgent_low_silence_interval");
    settings.alarm_low_enabled = (*doc)["alarm_low_enabled"].as<bool>();
    settings.alarm_low_mgdl = (*doc)["alarm_low_value"].as<int>();
    settings.alarm_low_snooze_minutes = (*doc)["alarm_low_snooze_interval"].as<int>();
    settings.alarm_low_alert_windows =
        loadAlertWindows(*doc, "alarm_low_alert_windows", "alarm_low_silence_interval");
    settings.alarm_high_enabled = (*doc)["alarm_high_enabled"].as<bool>();
    settings.alarm_high_mgdl = (*doc)["alarm_high_value"].as<int>();
    settings.alarm_high_snooze_minutes = (*doc)["alarm_high_snooze_interval"].as<int>();
    settings.alarm_high_alert_windows =
        loadAlertWindows(*doc, "alarm_high_alert_windows", "alarm_high_silence_interval");
    settings.alarm_high_melody = (*doc)["alarm_high_melody"].as<String>();
    settings.alarm_low_melody = (*doc)["alarm_low_melody"].as<String>();
    settings.alarm_urgent_low_melody = (*doc)["alarm_urgent_low_melody"].as<String>();
    settings.alarm_intensive_mode = (*doc)["alarm_intensive_mode"].as<bool>();

    settings.alarm_repeat_interval_seconds = (*doc)["alarm_repeat_interval_seconds"] | 300;
    if (!isValidAlarmRepeatInterval(settings.alarm_repeat_interval_seconds)) {
        DEBUG_PRINTLN("Invalid alarm repeat interval in config, defaulting to 300 seconds");
        settings.alarm_repeat_interval_seconds = 300;
    }

    // Additional WiFi
    settings.additional_wifi_enable = (*doc)["additional_wifi_enable"].as<bool>();
    settings.additional_wifi_type = (*doc)["additional_wifi_type"].as<String>();
    settings.additional_wifi_ssid = (*doc)["additional_ssid"].as<String>();
    settings.additional_wifi_username = (*doc)["additional_wifi_username"].as<String>();
    settings.additional_wifi_password = (*doc)["additional_wifi_password"].as<String>();

    // Custom hostname
    settings.custom_hostname_enable = (*doc)["custom_hostname_enable"].as<bool>();
    settings.custom_hostname = (*doc)["custom_hostname"].as<String>();

    // Custom No Data Timer
    settings.custom_nodatatimer_enable = (*doc)["custom_nodatatimer_enable"].as<bool>();
    settings.custom_nodatatimer = (*doc)["custom_nodatatimer"].as<int>();
    if (settings.custom_nodatatimer_enable == true && settings.custom_nodatatimer > 5 &&
        settings.custom_nodatatimer <= 60) {
        settings.bg_data_too_old_threshold_minutes = settings.custom_nodatatimer;
    } else {
        settings.bg_data_too_old_threshold_minutes = 20;  // default value
        if (settings.custom_nodatatimer_enable == true) {
            DEBUG_PRINTLN("Custom No Data Timer value is invalid, using default value of 20 minutes.");
        }
    }

    settings.data_old_color = displayColorFromString(
        (*doc)["data_old_color"].as<String>(), DISPLAY_COLOR::GRAY);

    // Web interface authentication
    settings.web_auth_enable = (*doc)["web_auth_enable"].as<bool>();
    settings.web_auth_password = (*doc)["web_auth_password"].as<String>();

    delete doc;

    this->settings = settings;
    return true;
}

bool SettingsManager_::saveSettingsToFile() {
    auto doc = readConfigJsonFile();
    if (doc == NULL)
        return false;

    (*doc)["ssid"] = settings.ssid;
    (*doc)["password"] = settings.wifi_password;

    (*doc)["low_mgdl"] = settings.bg_low_warn_limit;
    (*doc)["high_mgdl"] = settings.bg_high_warn_limit;
    (*doc)["low_urgent_mgdl"] = settings.bg_low_urgent_limit;
    (*doc)["high_urgent_mgdl"] = settings.bg_high_urgent_limit;

    (*doc)["units"] = settings.bg_units == BG_UNIT::MMOLL ? "mmol" : "mgdl";

    (*doc)["brightness_mode"] = settings.brightness_mode == BRIGHTNES_MODE::AUTO_LINEAR   ? "auto_linear"
                                : settings.brightness_mode == BRIGHTNES_MODE::AUTO_DIMMED ? "auto_dimmed"
                                                                                          : "manual";
    (*doc)["brightness_level"] = settings.brightness_level + 1;
    (*doc)["default_face"] = settings.default_clockface;
    (*doc)["face_cycle_enabled"] = settings.face_cycle_enabled;
    (*doc)["face_cycle_interval_seconds"] = settings.face_cycle_interval_seconds;
    (*doc).remove("face_cycle_faces");
    JsonArray faceCycleFaces = (*doc)["face_cycle_faces"].to<JsonArray>();
    for (int faceId : settings.face_cycle_faces) {
        faceCycleFaces.add(faceId);
    }

    String data_source = "no_source";
    switch (settings.bg_source) {
        case BG_SOURCE::NIGHTSCOUT:
            data_source = "nightscout";
            break;
        case BG_SOURCE::DEXCOM:
            data_source = "dexcom";
            break;
        case BG_SOURCE::MEDTRONIC:
            data_source = "medtronic";
            break;
        case BG_SOURCE::API:
            data_source = "api";
            break;
        case BG_SOURCE::LIBRELINKUP:
            data_source = "librelinkup";
            break;
        case BG_SOURCE::MEDTRUM:
            data_source = "medtrum";
            break;
        default:
            data_source = "no_source";
            break;
    }
    (*doc)["data_source"] = data_source;
    (*doc)["medtrum_email"] = settings.medtrum_email;
    (*doc)["medtrum_password"] = settings.medtrum_password;

    (*doc)["dexcom_username"] = settings.dexcom_username;
    (*doc)["dexcom_password"] = settings.dexcom_password;
    switch (settings.dexcom_server) {
        case DEXCOM_SERVER::US:
            (*doc)["dexcom_server"] = "us";
            break;
        case DEXCOM_SERVER::NON_US:
            (*doc)["dexcom_server"] = "ous";
            break;
        case DEXCOM_SERVER::JAPAN:
            (*doc)["dexcom_server"] = "jp";
            break;
        default:
            DEBUG_PRINTLN("Unknown Dexcom server, defaulting to US");
            (*doc)["dexcom_server"] = "ous";
            break;
    }

    (*doc)["librelinkup_email"] = settings.librelinkup_email;
    (*doc)["librelinkup_password"] = settings.librelinkup_password;
    (*doc)["librelinkup_region"] = settings.librelinkup_region;
    (*doc)["librelinkup_patient_id"] = settings.librelinkup_patient_id;

    (*doc)["nightscout_url"] = settings.nightscout_url;
    (*doc)["api_secret"] = settings.nightscout_api_key;
    (*doc)["nightscout_simplified_api"] = settings.nightscout_simplified_api;

    (*doc)["tz_libc"] = settings.tz_libc_value;
    (*doc)["time_format"] = settings.time_format == TIME_FORMAT::HOURS_12 ? "12" : "24";

    // save alarms data
    (*doc)["alarm_urgent_low_enabled"] = settings.alarm_urgent_low_enabled;
    (*doc)["alarm_urgent_low_value"] = settings.alarm_urgent_low_mgdl;
    (*doc)["alarm_urgent_low_snooze_interval"] = settings.alarm_urgent_low_snooze_minutes;
    writeAlertWindows(*doc, "alarm_urgent_low_alert_windows",
                      "alarm_urgent_low_silence_interval", settings.alarm_urgent_low_alert_windows);
    (*doc)["alarm_low_enabled"] = settings.alarm_low_enabled;
    (*doc)["alarm_low_value"] = settings.alarm_low_mgdl;
    (*doc)["alarm_low_snooze_interval"] = settings.alarm_low_snooze_minutes;
    writeAlertWindows(*doc, "alarm_low_alert_windows", "alarm_low_silence_interval",
                      settings.alarm_low_alert_windows);
    (*doc)["alarm_high_enabled"] = settings.alarm_high_enabled;
    (*doc)["alarm_high_value"] = settings.alarm_high_mgdl;
    (*doc)["alarm_high_snooze_interval"] = settings.alarm_high_snooze_minutes;
    writeAlertWindows(*doc, "alarm_high_alert_windows", "alarm_high_silence_interval",
                      settings.alarm_high_alert_windows);
    (*doc)["alarm_high_melody"] = settings.alarm_high_melody;
    (*doc)["alarm_low_melody"] = settings.alarm_low_melody;
    (*doc)["alarm_urgent_low_melody"] = settings.alarm_urgent_low_melody;
    (*doc)["alarm_intensive_mode"] = settings.alarm_intensive_mode;
    (*doc)["alarm_repeat_interval_seconds"] = settings.alarm_repeat_interval_seconds;

    // Additional WiFi
    (*doc)["additional_wifi_enable"] = settings.additional_wifi_enable;
    (*doc)["additional_wifi_type"] = settings.additional_wifi_type;
    (*doc)["additional_ssid"] = settings.additional_wifi_ssid;
    (*doc)["additional_wifi_username"] = settings.additional_wifi_username;
    (*doc)["additional_wifi_password"] = settings.additional_wifi_password;

    // Custom hostname
    (*doc)["custom_hostname_enable"] = settings.custom_hostname_enable;
    (*doc)["custom_hostname"] = settings.custom_hostname;

    // Custom No Data Timer
    (*doc)["custom_nodatatimer_enable"] = settings.custom_nodatatimer_enable;
    (*doc)["custom_nodatatimer"] = settings.custom_nodatatimer;
    (*doc)["data_old_color"] = toString(settings.data_old_color);

    // Web interface authentication
    (*doc)["web_auth_enable"] = settings.web_auth_enable;
    (*doc)["web_auth_password"] = settings.web_auth_password;

    if (trySaveJsonAsSettings(*doc) == false)
        return false;

    delete doc;

    return true;
}

bool SettingsManager_::trySaveJsonAsSettings(JsonDocument doc) {
    DEBUG_PRINTLN(doc.as<String>());
    auto file = LittleFS.open(CONFIG_JSON, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN("Failed to open config file for writing");
        return false;
    }

    auto result = file.print(doc.as<String>());

    file.close();
    if (!result) {
        return false;
    }

    return true;
}
