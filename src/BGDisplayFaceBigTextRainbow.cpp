#include "BGDisplayFaceBigTextRainbow.h"

#include "globals.h"

namespace {
constexpr unsigned long RAINBOW_REFRESH_INTERVAL_MS = 80;
constexpr uint8_t RAINBOW_BLEND_AMOUNT = 88;

uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((uint16_t)(red & 0xF8) << 8) | ((uint16_t)(green & 0xFC) << 3) | (blue >> 3);
}

uint16_t hsvToRgb565(uint8_t hue) {
    uint8_t region = hue / 43;
    uint8_t remainder = (hue - (region * 43)) * 6;
    uint8_t q = 255 - remainder;
    uint8_t t = remainder;

    switch (region) {
        case 0:
            return rgb565(255, t, 0);
        case 1:
            return rgb565(q, 255, 0);
        case 2:
            return rgb565(0, 255, t);
        case 3:
            return rgb565(0, q, 255);
        case 4:
            return rgb565(t, 0, 255);
        default:
            return rgb565(255, 0, q);
    }
}

uint8_t redFromRgb565(uint16_t color) { return ((color >> 11) & 0x1F) * 255 / 31; }
uint8_t greenFromRgb565(uint16_t color) { return ((color >> 5) & 0x3F) * 255 / 63; }
uint8_t blueFromRgb565(uint16_t color) { return (color & 0x1F) * 255 / 31; }

uint16_t blendRgb565(uint16_t baseColor, uint16_t overlayColor, uint8_t overlayAmount) {
    uint16_t baseAmount = 255 - overlayAmount;
    uint8_t red = (redFromRgb565(baseColor) * baseAmount + redFromRgb565(overlayColor) * overlayAmount) / 255;
    uint8_t green =
        (greenFromRgb565(baseColor) * baseAmount + greenFromRgb565(overlayColor) * overlayAmount) / 255;
    uint8_t blue =
        (blueFromRgb565(baseColor) * baseAmount + blueFromRgb565(overlayColor) * overlayAmount) / 255;

    return rgb565(red, green, blue);
}
}  // namespace

void BGDisplayFaceBigTextRainbow::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    showAnimatedReading(lastReading, dataIsOld);

    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1, dataIsOld, true, false);
    DisplayManager.update();
}

bool BGDisplayFaceBigTextRainbow::needsFrequentRefresh() const { return true; }

unsigned long BGDisplayFaceBigTextRainbow::getFrequentRefreshIntervalMs() const {
    return RAINBOW_REFRESH_INTERVAL_MS;
}

void BGDisplayFaceBigTextRainbow::showAnimatedReading(
    const GlucoseReading& reading, bool dataIsOld) const {
    String readingToDisplay = getPrintableReading(reading.sgv);
    uint16_t baseColor = dataIsOld ? BG_COLOR_OLD : getDisplayColorByBGValue(reading);
    uint8_t hueOffset = (millis() / 20) % 255;
    uint8_t textLength = readingToDisplay.length();
    int16_t x = 0;

    DisplayManager.setFont(FONT_TYPE::LARGE);

    for (uint8_t i = 0; i < textLength; i++) {
        char text[2] = {readingToDisplay[i], '\0'};
        uint8_t hue = map(i, 0, max((int)textLength, 1), 0, 255) + hueOffset;
        uint16_t color = dataIsOld ? baseColor : blendRgb565(baseColor, hsvToRgb565(hue), RAINBOW_BLEND_AMOUNT);

        DisplayManager.setTextColor(color);
        DisplayManager.printText(x, 7, text, TEXT_ALIGNMENT::LEFT, 2, false);
        x += DisplayManager.getTextWidth(text, 2);
    }
}
