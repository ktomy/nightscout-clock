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
    String tz_libc_value;
    TIME_FORMAT time_format;
};