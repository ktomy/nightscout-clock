// Emulator stand-in: the TC001's temperature/humidity sensor. Nothing on the display reads it.
#ifndef EMU_ADAFRUIT_SHT31_H
#define EMU_ADAFRUIT_SHT31_H

#include <Wire.h>

class Adafruit_SHT31 {
public:
    bool begin(uint8_t address = 0x44) { return true; }
    bool readBoth(float* temperature, float* humidity) {
        *temperature = 21.0f;
        *humidity = 40.0f;
        return true;
    }
};

#endif
