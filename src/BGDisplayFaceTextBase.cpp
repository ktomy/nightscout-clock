#include "BGDisplayFaceTextBase.h"
#include "BGDisplayManager.h"
#include "globals.h"
#include <map>

void BGDisplayFaceTextBase::showReading(const GlucoseReading reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment,
                                        FONT_TYPE font) const {

    String readingToDisplay = getPrintableReading(reading);

    SetDisplayColorByBGValue(reading);
    DisplayManager.setFont(font);

    DisplayManager.printText(x, y, readingToDisplay.c_str(), alignment, 2);
}

void BGDisplayFaceTextBase::SetDisplayColorByBGValue(const GlucoseReading &reading) const {

    auto bgLevel = BGDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    auto textColor = COLOR_GRAY;

    switch (bgLevel) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::URGENT_HIGH:
            textColor = COLOR_RED;
            break;
        case BG_LEVEL::WARNING_LOW:
        case BG_LEVEL::WARNING_HIGH:
            textColor = COLOR_YELLOW;
            break;
        case BG_LEVEL::NORMAL:
            textColor = COLOR_GREEN;
            break;
    }

    DisplayManager.setTextColor(textColor);
}

String BGDisplayFaceTextBase::getPrintableReading(const GlucoseReading &reading) const {
    String readingToDisplay = "";
    if (SettingsManager.settings.bg_units == BG_UNIT::MGDL) {
        readingToDisplay += String(reading.sgv);
    } else {
        char buffer[10];
        sprintf(buffer, "%.1f", round((float)reading.sgv / 1.8) / 10);
        readingToDisplay += String(buffer);
    }
    return readingToDisplay;
}

#pragma region Show arrow

// Glucose trends
const uint8_t symbol_doubleUp[] PROGMEM = {
    0x50, 0xF8, 0x50, 0x50, 0x50,
};
const uint8_t symbol_singleUp[] PROGMEM = {
    0x20, 0x70, 0xA8, 0x20, 0x20,
};
const uint8_t symbol_fortyFiveUp[] PROGMEM = {
    0x38, 0x18, 0x28, 0x40, 0x80,
};
const uint8_t symbol_flat[] PROGMEM = {
    0x20, 0x10, 0xF8, 0x10, 0x20,
};
const uint8_t symbol_fortyFiveDown[] PROGMEM = {
    0x80, 0x40, 0x28, 0x18, 0x38,
};
const uint8_t symbol_singleDown[] PROGMEM = {
    0x20, 0x20, 0xA8, 0x70, 0x20,
};
const uint8_t symbol_doubleDown[] PROGMEM = {
    0x50, 0x50, 0x50, 0xF8, 0x50,
};

const uint8_t symbol_empty[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00,
};

const std::map<BG_TREND, const uint8_t *> glucoseTrendSymbols = {
    {BG_TREND::NONE, symbol_empty},
    {BG_TREND::SINGLE_UP, symbol_singleUp},
    {BG_TREND::FORTY_FIVE_UP, symbol_fortyFiveUp},
    {BG_TREND::FLAT, symbol_flat},
    {BG_TREND::FORTY_FIVE_DOWN, symbol_fortyFiveDown},
    {BG_TREND::SINGLE_DOWN, symbol_singleDown},
    {BG_TREND::DOUBLE_DOWN, symbol_doubleDown},
    {BG_TREND::NOT_COMPUTABLE, symbol_empty},
    {BG_TREND::RATE_OUT_OF_RANGE, symbol_doubleUp},
};

void BGDisplayFaceTextBase::showTrendArrow(const GlucoseReading reading, int16_t x, int16_t y) const {

    DisplayManager.drawBitmap(x, y, glucoseTrendSymbols.at(reading.trend), 5, 5, COLOR_WHITE);
}

#pragma endregion Show arrow
