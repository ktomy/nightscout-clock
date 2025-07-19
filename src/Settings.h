#include <Arduino.h>

#include "enums.h"

class Settings {
public:
    String ssid;
    String wifi_password;
    String hostname;
    String nightscout_url;
    String nightscout_api_key;
    BG_UNIT bg_units;
    int bg_low_warn_limit;
    int bg_high_warn_limit;
    int bg_low_urgent_limit;
    int bg_high_urgent_limit;
    bool auto_brightness;
    int brightness_level;
    int default_clockface;
    BG_SOURCE bg_source;
    String dexcom_username;
    String dexcom_password;
    DEXCOM_SERVER dexcom_server;
    String librelinkup_email;
    String librelinkup_password;
    String librelinkup_region;
    String tz_libc_value;
    TIME_FORMAT time_format;
    bool alarm_urgent_low_enabled;
    int alarm_urgent_low_mgdl;
    int alarm_urgent_low_snooze_minutes;
    String alarm_urgent_low_silence_interval;
    bool alarm_low_enabled;
    int alarm_low_mgdl;
    int alarm_low_snooze_minutes;
    String alarm_low_silence_interval;
    bool alarm_high_enabled;
    int alarm_high_mgdl;
    int alarm_high_snooze_minutes;
    String alarm_high_silence_interval;
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
    bool dimmer_mode_enable = false;
    bool brighter_mode_enable = false;
};
