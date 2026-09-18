// Stand-in for FastLED 3.6.0 with only what Framebuffer_GFX, FastLED_NeoMatrix and DisplayManager use.
// The color math is FastLED's, so `wire` holds the bytes the ESP32 sends to the LEDs.
#ifndef EMU_FASTLED_H
#define EMU_FASTLED_H

#include <Arduino.h>

typedef uint8_t fract8;

// lib8tion/scale8.h with FASTLED_SCALE8_FIXED 1, C path.
inline uint8_t scale8(uint8_t i, fract8 scale) { return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8; }

struct CRGB {
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
        uint8_t raw[3];
    };

    CRGB() = default;
    CRGB(uint8_t ir, uint8_t ig, uint8_t ib) {
        r = ir;
        g = ig;
        b = ib;
    }
    CRGB(uint32_t colorcode) { *this = colorcode; }
    CRGB& operator=(uint32_t colorcode) {
        r = (colorcode >> 16) & 0xFF;
        g = (colorcode >> 8) & 0xFF;
        b = (colorcode >> 0) & 0xFF;
        return *this;
    }
};

struct CHSV {
    union {
        struct {
            uint8_t hue;
            uint8_t sat;
            uint8_t val;
        };
        uint8_t raw[3];
    };

    CHSV() = default;
    CHSV(uint8_t ih, uint8_t is, uint8_t iv) {
        hue = ih;
        sat = is;
        val = iv;
    }
};

void hsv2rgb_spectrum(const CHSV& hsv, CRGB& rgb);
uint8_t applyGamma_video(uint8_t brightness, float gamma);
inline void random16_set_seed(uint16_t) {}

template <uint8_t DATA_PIN, int RGB_ORDER = 0>
class NEOPIXEL {};

class CFastLED {
public:
    template <template <uint8_t, int> class CHIPSET, uint8_t DATA_PIN>
    CFastLED& addLeds(CRGB* data, int nLeds) {
        leds = data;
        numLeds = nLeds;
        return *this;
    }
    void setBrightness(uint8_t scale) { brightness = scale; }
    uint8_t getBrightness() { return brightness; }
    void show();

    CRGB* leds = nullptr;
    int numLeds = 0;
    uint8_t brightness = 255;
    uint8_t wire[3 * 32 * 8] = {};
};

extern CFastLED FastLED;

#endif
