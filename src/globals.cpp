#include "globals.h"

uint16_t LDR_RAW;
String TIME_FORMAT = "%H:%M:%S";
String DATE_FORMAT = "%d.%m.%y";
int BACKGROUND_EFFECT=-1;
bool START_ON_MONDAY;
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
bool AUTO_BRIGHTNESS = true;
int BRIGHTNESS = 120;
int BRIGHTNESS_PERCENT;
bool MATRIX_OFF;
uint8_t MIN_BRIGHTNESS = 2;
uint8_t MAX_BRIGHTNESS = 180;

