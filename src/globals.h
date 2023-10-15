#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <IPAddress.h>
#define DEBUG

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

#define CONFIG_JSON "/config.json"
#define CONFIG_JSON_FACTORY "/config_initial.json"
#define WIFI_CONNECT_TIMEOUT 15000
#define AP_MODE_PASSWORD ""
#define HOSTNAME_PREFIX "nsclock"
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"
#define DEFAULT_TIMEZONE "UTC0"

extern bool BLOCK_NAVIGATION;
extern float TEMP_OFFSET;
extern float HUM_OFFSET;
extern int BRIGHTNESS;
extern int BRIGHTNESS_PERCENT;
extern bool AUTO_BRIGHTNESS;
extern float CURRENT_TEMP;
extern float CURRENT_HUM;
extern float CURRENT_LUX;
extern bool IS_CELSIUS;
extern bool SENSOR_READING;
extern uint16_t LDR_RAW;
extern String TIME_FORMAT;
extern String DATE_FORMAT;
extern int BACKGROUND_EFFECT;
extern bool START_ON_MONDAY;
extern uint8_t BATTERY_PERCENT;
extern uint16_t BATTERY_RAW;
extern bool MATRIX_OFF;
extern uint8_t MIN_BRIGHTNESS;
extern uint8_t MAX_BRIGHTNESS;

#endif // Globals_H
