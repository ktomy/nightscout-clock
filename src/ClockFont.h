#ifndef CLOCKFONT_H
#define CLOCKFONT_H

#include <Arduino.h>
#include <FastLED_NeoMatrix.h>
#include <map>

struct ClockFont {
    GFXfont font;
    std::map<char, uint16_t> charSizeMap;
};

#endif