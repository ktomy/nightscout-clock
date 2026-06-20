#include "BGDisplayFaceTextBase.h"

#include <map>

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceTextBase::showReading(
    const GlucoseReading reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment, FONT_TYPE font,
    bool isOld) const {
    String readingToDisplay = getPrintableReading(reading.sgv);
    if (!isOld) {
        SetDisplayColorByBGValue(reading);
    } else {
        DisplayManager.setTextColor(BG_COLOR_OLD);
    }

    DisplayManager.setFont(font);

    DisplayManager.printText(x, y, readingToDisplay.c_str(), alignment, 2);
}

void BGDisplayFaceTextBase::SetDisplayColorByBGValue(const GlucoseReading& reading) const {
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    auto textColor = COLOR_GRAY;

    switch (bgLevel) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::URGENT_HIGH:
            textColor = BG_COLOR_URGENT;
            break;
        case BG_LEVEL::WARNING_LOW:
        case BG_LEVEL::WARNING_HIGH:
            textColor = BG_COLOR_WARNING;
            break;
        case BG_LEVEL::NORMAL:
            textColor = BG_COLOR_NORMAL;
            break;
    }

    DisplayManager.setTextColor(textColor);
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
    const GlucoseReading reading, int16_t x, int16_t y, bool dataIsOld) const {
    uint16_t color = COLOR_WHITE;
    if (dataIsOld) {
        color = BG_COLOR_OLD;
    }

    DisplayManager.drawBitmap(x, y, glucoseTrendSymbols.at(reading.trend), 5, 5, color);
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
    DisplayManager.update();
}
