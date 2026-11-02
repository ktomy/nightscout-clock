// Stand-in for the ESP32 Arduino core header. String, Print and Stream are the core's own sources,
// copied in by build.sh; only the hardware-facing parts are replaced.
#ifndef EMU_ARDUINO_H
#define EMU_ARDUINO_H

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp32-hal-log.h"
#include "pgmspace.h"

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x01
#define OUTPUT 0x03

#define PI 3.1415926535897932384626433832795
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

typedef bool boolean;
typedef uint8_t byte;
typedef unsigned int word;

#ifdef __cplusplus
extern "C" {
#endif
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void yield(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
uint16_t analogRead(uint8_t pin);
void analogReadResolution(uint8_t bits);
double ledcSetup(uint8_t channel, double frequency, uint8_t resolutionBits);
void ledcAttachPin(uint8_t pin, uint8_t channel);
void ledcDetachPin(uint8_t pin);
void ledcWrite(uint8_t channel, uint32_t duty);
double ledcWriteTone(uint8_t channel, double frequency);

#define MALLOC_CAP_INTERNAL (1 << 11)
#define MALLOC_CAP_DMA (1 << 3)
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <algorithm>
#include <cmath>

using ::round;
using std::abs;
using std::isinf;
using std::isnan;
using std::max;
using std::min;

#include "Print.h"
#include "Stream.h"
#include "WString.h"

inline bool isDigit(int c) { return isdigit(c) != 0; }
inline bool isAlpha(int c) { return isalpha(c) != 0; }
inline bool isAlphaNumeric(int c) { return isalnum(c) != 0; }
inline bool isSpace(int c) { return isspace(c) != 0; }

class EspClass {
public:
    [[noreturn]] void restart();
    uint32_t getFreeHeap() { return 0; }
};
extern EspClass ESP;

// Serial output is discarded.
class HardwareSerial : public Stream {
public:
    void begin(unsigned long) {}
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t write(uint8_t) override { return 1; }
    using Print::write;
};
extern HardwareSerial Serial;
#endif

#endif
