#include "BGDisplayFaceRainbowSparkle.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {
const uint8_t PROGMEM SPARKLE_HAPPY_A[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10000001,
    0b10100101, 0b10011001, 0b01000010, 0b00111100};

const uint8_t PROGMEM SPARKLE_HAPPY_B[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10000001,
    0b10000001, 0b10111101, 0b01000010, 0b00111100};

const uint8_t PROGMEM SPARKLE_SHOCKED[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10100101,
    0b10000001, 0b10011001, 0b10011001, 0b00111100};

const uint8_t PROGMEM SPARKLE_DIZZY[] = {
    0b00111100, 0b01000010, 0b10011001, 0b00100100,
    0b10000001, 0b01011010, 0b01000010, 0b00111100};
}  // namespace

void BGDisplayFaceRainbowSparkle::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    showAnimatedReading(lastReading, dataIsOld);
    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1, dataIsOld);
}

unsigned long BGDisplayFaceRainbowSparkle::getAnimationStepMillis() const { return 60; }

void BGDisplayFaceRainbowSparkle::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    auto lastReading = readings.back();
    DisplayManager.clearMatrix();
    showAnimatedReading(lastReading, false);
    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1, false);
}

void BGDisplayFaceRainbowSparkle::showAnimatedReading(
    const GlucoseReading& reading, bool dataIsOld) const {
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    bool isFallingFast =
        (reading.trend == BG_TREND::DOUBLE_DOWN || reading.trend == BG_TREND::SINGLE_DOWN);

    const uint8_t* faceBmp = (millis() / 300 % 2 == 0) ? SPARKLE_HAPPY_A : SPARKLE_HAPPY_B;
    uint16_t faceColor = COLOR_GREEN;

    if (isFallingFast) {
        faceBmp = SPARKLE_SHOCKED;
        faceColor = COLOR_YELLOW;
    } else if (bgLevel == BG_LEVEL::URGENT_HIGH || bgLevel == BG_LEVEL::WARNING_HIGH) {
        faceBmp = SPARKLE_DIZZY;
        faceColor = 0xF81F;  // Magenta
    } else if (bgLevel == BG_LEVEL::URGENT_LOW || bgLevel == BG_LEVEL::WARNING_LOW) {
        faceBmp = SPARKLE_SHOCKED;
        faceColor = COLOR_RED;
    }

    if (dataIsOld) {
        faceColor = getDataOldColor();
    }

    DisplayManager.drawBitmap(0, 0, faceBmp, 8, 8, faceColor);

    String readingToDisplay = getPrintableReading(reading.sgv);
    uint8_t hueOffset = (millis() / 25) % 255;
    uint8_t textLen = readingToDisplay.length();
    int16_t x = 9;

    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    for (uint8_t i = 0; i < textLen; i++) {
        char buf[2] = {readingToDisplay[i], '\0'};
        uint8_t charHue = hueOffset + (i * 35);
        uint16_t color = dataIsOld ? getDataOldColor() : DisplayManager.hsvToRgb565(charHue);
        DisplayManager.setTextColor(color);
        DisplayManager.printText(x, 6, buf, TEXT_ALIGNMENT::LEFT, 2);
        x += DisplayManager.getTextWidth(buf, 2);
    }
}
