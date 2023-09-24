#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <IPAddress.h>
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTLN(x)    \
  {                         \
    Serial.print("[");      \
    Serial.print(millis()); \
    Serial.print("] [");    \
    Serial.print(__func__); \
    Serial.print("]: ");    \
    Serial.println(x);      \
  }
#define DEBUG_PRINTF(format, ...)                                                        \
  {                                                                                      \
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
#define WIFI_CONNECT_TIMEOUT 15000
#define AP_MODE_PASSWORD ""
#define HOSTNAME_PREFIX "nsclock"
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"
#define DEFAULT_TIMEZONE "UTC0"

#endif // Globals_H
