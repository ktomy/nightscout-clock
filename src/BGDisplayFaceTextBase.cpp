#include "BGDisplayFaceTextBase.h"

#include <map>

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceTextBase::showReading(
    const GlucoseReading reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment, FONT_TYPE font,
    bool isOld) const {
    DisplayManager.setTextColor(isOld ? getDataOldColor() : getColorByBGValue(reading));
    printReading(reading, x, y, alignment, font);
}

void BGDisplayFaceTextBase::printReading(
    const GlucoseReading& reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment,
    FONT_TYPE font) const {
    String readingToDisplay = getPrintableReading(reading.sgv);
    DisplayManager.setFont(font);

    DisplayManager.printText(x, y, readingToDisplay.c_str(), alignment, 2);
}

uint16_t BGDisplayFaceTextBase::getColorByBGValue(const GlucoseReading& reading) const {
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    return getBandColor(bgLevel);
}

String BGDisplayFaceTextBase::getPrintableReading(const int sgv) const {
    return formatDisplayTenths(toDisplayTenths(sgv));
}

// `sgv` is always raw mg/dL, exactly as delivered by the data source - that never
// changes anywhere in the codebase. This helper only produces a *local, display-scale*
// representation of a reading, used to compare/diff readings the same way they are
// shown on screen. It is never stored and never passed back into GlucoseReading.sgv.
//
// The result is scaled by 10 ("tenths") so it stays an integer that callers can use in
// arithmetic (e.g. min/max/subtraction) before paying the string-formatting cost:
//  - mg/dL: the displayed resolution is whole mg/dL, so this is the identity function.
//  - mmol/L: mg/dL / 18 = mmol/L; multiplying by 10 and rounding gives the same value
//    that gets printed with one decimal (e.g. sgv=193 -> 107, printed later as "10.7").
int BGDisplayFaceTextBase::toDisplayTenths(const int sgv) const {
    if (SettingsManager.settings.bg_units == BG_UNIT::MGDL) {
        return sgv;
    }
    return round((float)sgv / 1.8);
}

// Formats a value produced by toDisplayTenths() back into the string shown on screen
// (e.g. 107 -> "10.7"). Kept separate from toDisplayTenths() so callers can do integer
// arithmetic on the tenths value first and only format the final result.
String BGDisplayFaceTextBase::formatDisplayTenths(const int tenths) const {
    if (SettingsManager.settings.bg_units == BG_UNIT::MGDL) {
        return String(tenths);
    }
    char buffer[10];
    sprintf(buffer, "%.1f", (float)tenths / 10);
    return String(buffer);
}

#pragma region Show arrow

// Glucose trends
const uint8_t symbol_doubleUp[] PROGMEM = {
    0b01010000,
    0b11111000,
    0b01010000,
    0b01010000,
    0b01010000,
};
const uint8_t symbol_singleUp[] PROGMEM = {
    0b00100000,
    0b01110000,
    0b10101000,
    0b00100000,
    0b00100000,
};
const uint8_t symbol_fortyFiveUp[] PROGMEM = {
    0b00111000,
    0b00011000,
    0b00101000,
    0b01000000,
    0b10000000,
};
const uint8_t symbol_flat[] PROGMEM = {
    0b00100000,
    0b00010000,
    0b11111000,
    0b00010000,
    0b00100000,
};
const uint8_t symbol_fortyFiveDown[] PROGMEM = {
    0b10000000,
    0b01000000,
    0b00101000,
    0b00011000,
    0b00111000,
};
const uint8_t symbol_singleDown[] PROGMEM = {
    0b00100000,
    0b00100000,
    0b10101000,
    0b01110000,
    0b00100000,
};
const uint8_t symbol_doubleDown[] PROGMEM = {
    0b01010000,
    0b01010000,
    0b01010000,
    0b11111000,
    0b01010000,
};

const uint8_t symbol_empty[] PROGMEM = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
};

// Drawn in the arrow's place once the reading is too old: the clock has no current trend to show.
const uint8_t symbol_dataOld[] PROGMEM = {
    0b10001000,
    0b01010000,
    0b00100000,
    0b01010000,
    0b10001000,
};

const std::map<BG_TREND, const uint8_t*> glucoseTrendSymbols = {
    {BG_TREND::NONE, symbol_empty},
    {BG_TREND::DOUBLE_UP, symbol_doubleUp},
    {BG_TREND::SINGLE_UP, symbol_singleUp},
    {BG_TREND::FORTY_FIVE_UP, symbol_fortyFiveUp},
    {BG_TREND::FLAT, symbol_flat},
    {BG_TREND::FORTY_FIVE_DOWN, symbol_fortyFiveDown},
    {BG_TREND::SINGLE_DOWN, symbol_singleDown},
    {BG_TREND::DOUBLE_DOWN, symbol_doubleDown},
    {BG_TREND::NOT_COMPUTABLE, symbol_empty},
    {BG_TREND::RATE_OUT_OF_RANGE, symbol_doubleUp},
};

void BGDisplayFaceTextBase::showTrendArrow(
    const GlucoseReading reading, int16_t x, int16_t y, bool dataIsOld, uint16_t freshColor) const {
    if (dataIsOld) {
        DisplayManager.drawBitmap(x, y, symbol_dataOld, 5, 5, getDataOldColor());
        return;
    }

    DisplayManager.drawBitmap(x, y, glucoseTrendSymbols.at(reading.trend), 5, 5, freshColor);
}

#pragma endregion Show arrow

void BGDisplayFaceTextBase::showTrendVerticalLine(int x, BG_TREND trend, bool dataIsOld) const {
    if (dataIsOld) {
        trend = BG_TREND::NONE;
    }
    switch (trend) {
        case BG_TREND::DOUBLE_UP:
            DisplayManager.drawPixel(x, 0, COLOR_RED);
            DisplayManager.drawPixel(x, 1, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case BG_TREND::DOUBLE_DOWN:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            DisplayManager.drawPixel(x, 6, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 7, COLOR_RED);
            break;
        case BG_TREND::SINGLE_UP:
            DisplayManager.drawPixel(x, 1, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case BG_TREND::SINGLE_DOWN:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            DisplayManager.drawPixel(x, 6, COLOR_YELLOW);
            break;
        case BG_TREND::FORTY_FIVE_UP:
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case BG_TREND::FORTY_FIVE_DOWN:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            break;
        case BG_TREND::FLAT:
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            break;
        default:
            DisplayManager.setTextColor(COLOR_BLACK);
            break;
    }
}
