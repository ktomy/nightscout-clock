#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <IPAddress.h>

#include "enums.h"

#define VERSION "0.30.0"

#ifdef DEBUG
#define DEBUG_PRINTLN(x)        \
    {                           \
        Serial.print("[");      \
        Serial.print(millis()); \
        Serial.print("] [");    \
        Serial.print(__func__); \
        Serial.print("]: ");    \
        Serial.println(x);      \
    }
#define DEBUG_PRINTF(format, ...)                                                            \
    {                                                                                        \
        String formattedMessage = "[" + String(millis()) + "] [" + String(__func__) + "]: "; \
        Serial.print(formattedMessage);                                                      \
        Serial.printf(format, ##__VA_ARGS__);                                                \
        Serial.println();                                                                    \
    }
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(format, ...)
#endif

// How many clock faces BGDisplayManager registers. Face ids are validated against this both in
// the settings API and when loading a config, so this must be updated when a face is added.
#define CLOCK_FACE_COUNT 6
#define CONFIG_JSON "/config.json"
#define CONFIG_JSON_FACTORY "/config_initial.json"
#define WIFI_CONNECT_TIMEOUT 15000
#define AP_MODE_PASSWORD ""
#define HOSTNAME_PREFIX "nsclock"
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"
#define DEFAULT_TIMEZONE "UTC0"
#define TIME_SYNC_INTERVAL 86400
// Defined from DISPLAY_COLOR so the RGB565 codes are declared in exactly one place.
#define COLOR_RED static_cast<uint16_t>(DISPLAY_COLOR::RED)
#define COLOR_GREEN static_cast<uint16_t>(DISPLAY_COLOR::GREEN)
#define COLOR_YELLOW static_cast<uint16_t>(DISPLAY_COLOR::YELLOW)
#define COLOR_WHITE static_cast<uint16_t>(DISPLAY_COLOR::WHITE)
#define COLOR_GRAY static_cast<uint16_t>(DISPLAY_COLOR::GRAY)
#define COLOR_BLACK static_cast<uint16_t>(DISPLAY_COLOR::BLACK)
#define COLOR_BLUE static_cast<uint16_t>(DISPLAY_COLOR::BLUE)
#define COLOR_CYAN static_cast<uint16_t>(DISPLAY_COLOR::CYAN)
#define COLOR_MAGENTA static_cast<uint16_t>(DISPLAY_COLOR::MAGENTA)

#define BG_COLOR_NORMAL COLOR_GREEN
#define BG_COLOR_WARNING COLOR_YELLOW
#define BG_COLOR_URGENT COLOR_RED

extern bool BLOCK_NAVIGATION;
extern float TEMP_OFFSET;
extern float HUM_OFFSET;
extern float CURRENT_TEMP;
extern float CURRENT_HUM;
extern float CURRENT_LUX;
extern bool IS_CELSIUS;
extern bool SENSOR_READING;
extern uint16_t LDR_RAW;
extern uint8_t BATTERY_PERCENT;
extern uint16_t BATTERY_RAW;
extern bool MATRIX_OFF;
extern uint8_t MIN_BRIGHTNESS;
extern uint8_t MAX_BRIGHTNESS;
extern const String sound_urgent_low PROGMEM;
extern const String sound_low PROGMEM;
extern const String sound_high PROGMEM;
extern const String sound_boot PROGMEM;
#endif  // Globals_H
