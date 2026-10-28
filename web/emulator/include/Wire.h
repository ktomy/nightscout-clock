// Emulator stand-in: no I2C bus.
#ifndef EMU_WIRE_H
#define EMU_WIRE_H

#include <Arduino.h>

class TwoWire {
public:
    bool begin(int sda = -1, int scl = -1, uint32_t frequency = 0) { return true; }
};

extern TwoWire Wire;

#endif
