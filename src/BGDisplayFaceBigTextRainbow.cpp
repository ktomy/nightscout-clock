#include "BGDisplayFaceBigTextRainbow.h"

#include "globals.h"

namespace {
constexpr unsigned long RAINBOW_STEP_MS = 80;
constexpr uint8_t RAINBOW_BLEND_AMOUNT = 192;
constexpr unsigned long STALE_BLINK_INTERVAL_MS = 500;

uint8_t redFromRgb565(uint16_t color) { return ((color >> 11) & 0x1F) * 255 / 31; }
uint8_t greenFromRgb565(uint16_t color) { return ((color >> 5) & 0x3F) * 255 / 63; }
uint8_t blueFromRgb565(uint16_t color) { return (color & 0x1F) * 255 / 31; }

uint16_t blendRgb565(uint16_t baseColor, uint16_t overlayColor, uint8_t overlayAmount) {
    uint16_t baseAmount = 255 - overlayAmount;
    uint8_t red =
        (redFromRgb565(baseColor) * baseAmount + redFromRgb565(overlayColor) * overlayAmount) / 255;
    uint8_t green = (greenFromRgb565(baseColor) * baseAmount +
                     greenFromRgb565(overlayColor) * overlayAmount) /
                    255;
    uint8_t blue = (blueFromRgb565(baseColor) * baseAmount +
                    blueFromRgb565(overlayColor) * overlayAmount) /
                   255;

    return DisplayManager.rgb565(red, green, blue);
}
}  // namespace

void BGDisplayFaceBigTextRainbow::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    // Blink while the data is stale, driven by the per-second tick.
    const bool blinkVisible = !dataIsOld || (millis() / STALE_BLINK_INTERVAL_MS) % 2 == 0;
    DisplayManager.clearMatrix();
    if (blinkVisible) {
        showAnimatedReading(lastReading, dataIsOld);
        showTrendArrow(
            lastReading, MATRIX_WIDTH - 5, 1, dataIsOld, getColorByBGValue(lastReading));
    }
}

unsigned long BGDisplayFaceBigTextRainbow::getAnimationStepMillis() const {
    return RAINBOW_STEP_MS;
}

void BGDisplayFaceBigTextRainbow::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    auto lastReading = readings.back();
    DisplayManager.clearMatrix();
    showAnimatedReading(lastReading, false);
    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1, false, getColorByBGValue(lastReading));
}

RenderDecision BGDisplayFaceBigTextRainbow::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason == RenderReason::TIME_TICK) {
        return RenderDecision::FULL;
    }
    return BGDisplayFace::getRenderDecision(ctx);
}

bool BGDisplayFaceBigTextRainbow::ticksEverySecond() const { return true; }

void BGDisplayFaceBigTextRainbow::showAnimatedReading(
    const GlucoseReading& reading, bool dataIsOld) const {
    String readingToDisplay = getPrintableReading(reading.sgv);
    uint16_t baseColor = dataIsOld ? getDataOldColor() : getColorByBGValue(reading);
    uint8_t hueOffset = (millis() / 20) % 255;
    uint8_t textLength = readingToDisplay.length();
    int16_t x = 0;
    DisplayManager.setFont(FONT_TYPE::LARGE);

    for (uint8_t i = 0; i < textLength; i++) {
        char text[2] = {readingToDisplay[i], '\0'};
        uint8_t hue = map(i, 0, max((int)textLength, 1), 0, 255) + hueOffset;
        uint16_t color = dataIsOld ? baseColor
                                   : blendRgb565(
                                         baseColor, DisplayManager.hsvToRgb565(hue),
                                         RAINBOW_BLEND_AMOUNT);

        DisplayManager.setTextColor(color);
        DisplayManager.printText(x, 7, text, TEXT_ALIGNMENT::LEFT, 2);
        x += DisplayManager.getTextWidth(text, 2);
    }
}
