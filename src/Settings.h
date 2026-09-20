#ifndef Settings_h
#define Settings_h

#include <Arduino.h>

#include <vector>

#include "SettingsAlarm.h"
#include "SettingsSchedule.h"
#include "enums.h"

class Settings {
public:
    String ssid;
    String wifi_password;
    String hostname;
    String nightscout_url;
    String nightscout_api_key;
    bool nightscout_simplified_api;
    BG_UNIT bg_units;
    int bg_low_warn_limit;
    int bg_high_warn_limit;
    int bg_low_urgent_limit;
    int bg_high_urgent_limit;
    BRIGHTNES_MODE brightness_mode;
    int brightness_level;
    int default_clockface;
    bool face_cycle_enabled = false;
    std::vector<int> inactive_faces;
    int face_cycle_interval_seconds = 60;
    bool face_schedule_enabled = false;
    std::vector<FaceScheduleEntry> face_schedule;
    BG_SOURCE bg_source;
    String dexcom_username;
    String dexcom_password;
    DEXCOM_SERVER dexcom_server;
    String librelinkup_email;
    String librelinkup_password;
    String librelinkup_region;
    String librelinkup_patient_id;
    // Medtrum Easy Follow
    String medtrum_email;
    String medtrum_password;
    String tz_libc_value;
    TIME_FORMAT time_format;
    bool alarm_urgent_low_enabled;
    int alarm_urgent_low_mgdl;
    int alarm_urgent_low_snooze_minutes;
    std::vector<AlertWindow> alarm_urgent_low_alert_windows;
    bool alarm_low_enabled;
    int alarm_low_mgdl;
    int alarm_low_snooze_minutes;
    std::vector<AlertWindow> alarm_low_alert_windows;
    bool alarm_high_enabled;
    int alarm_high_mgdl;
    int alarm_high_snooze_minutes;
    std::vector<AlertWindow> alarm_high_alert_windows;
    String alarm_high_melody;
    String alarm_low_melody;
    String alarm_urgent_low_melody;
    bool additional_wifi_enable;
    String additional_wifi_type;
    String additional_wifi_ssid;
    String additional_wifi_username;
    String additional_wifi_password;
    bool custom_hostname_enable;
    String custom_hostname;
    bool custom_nodatatimer_enable;
    int custom_nodatatimer;
    int bg_data_too_old_threshold_minutes = 20;
    DISPLAY_COLOR data_old_color = DISPLAY_COLOR::GRAY;
    bool alarm_intensive_mode;
    int alarm_repeat_interval_seconds = 300;
    bool web_auth_enable;
    String web_auth_password;
};

#endif
