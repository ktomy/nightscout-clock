#include "globals.h"

uint16_t LDR_RAW;
bool BLOCK_NAVIGATION = false;
float HUM_OFFSET;
float TEMP_OFFSET = -9;
float CURRENT_HUM;
float CURRENT_LUX;
float CURRENT_TEMP;
bool IS_CELSIUS;
bool SENSOR_READING = true;
uint8_t BATTERY_PERCENT;
uint16_t BATTERY_RAW;
int BRIGHTNESS = 120;
int BRIGHTNESS_PERCENT;
bool MATRIX_OFF;
uint8_t MIN_BRIGHTNESS = 2;
uint8_t MAX_BRIGHTNESS = 180;
const String sound_urgent_low PROGMEM = "urgent_low:d=4,o=5,b=230:4e6,4p,4e6,4p,4e6,4p,4e6";
const String sound_low PROGMEM = "low:d=4,o=5,b=200:4e5,4p,4e5,4p,4e5";
const String sound_high PROGMEM = "high:d=4,o=5,b=125:4e7,p,4e7";
const String sound_boot PROGMEM = "boot:d=4,o=5,b=320:32c,32e,32g";
int BG_DATA_OLD_OFFSET_MINUTES = 20;
