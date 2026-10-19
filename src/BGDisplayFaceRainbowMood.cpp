#include "BGDisplayFaceRainbowMood.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {
// The reading is centered between the smiley and the trend arrow.
constexpr int16_t SMILEY_WIDTH = 8;
constexpr int16_t ARROW_X = MATRIX_WIDTH - 5;

const uint8_t PROGMEM MOOD_HAPPY_A[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10000001,
    0b10100101, 0b10011001, 0b01000010, 0b00111100};

const uint8_t PROGMEM MOOD_HAPPY_B[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10000001,
    0b10000001, 0b10111101, 0b01000010, 0b00111100};

// The shocked mouth pulses between big and small.
const uint8_t PROGMEM MOOD_SHOCKED_A[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10100101,
    0b10000001, 0b10011001, 0b01011010, 0b00111100};

const uint8_t PROGMEM MOOD_SHOCKED_B[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10100101,
    0b10000001, 0b10011001, 0b01000010, 0b00111100};

// The dizzy eyes swap rows, so they seem to spin.
const uint8_t PROGMEM MOOD_DIZZY_A[] = {
    0b00111100, 0b01000010, 0b10011001, 0b10100101,
    0b10000001, 0b01011010, 0b01000010, 0b00111100};

const uint8_t PROGMEM MOOD_DIZZY_B[] = {
    0b00111100, 0b01000010, 0b10100101, 0b10011001,
    0b10000001, 0b01011010, 0b01000010, 0b00111100};
}  // namespace

void BGDisplayFaceRainbowMood::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    showAnimatedReading(lastReading, dataIsOld);
    showTrendArrow(lastReading, ARROW_X, 1, dataIsOld);
}

unsigned long BGDisplayFaceRainbowMood::getAnimationStepMillis() const { return 60; }

void BGDisplayFaceRainbowMood::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    auto lastReading = readings.back();
    DisplayManager.clearMatrix();
    showAnimatedReading(lastReading, false);
    showTrendArrow(lastReading, ARROW_X, 1, false);
}

void BGDisplayFaceRainbowMood::showAnimatedReading(
    const GlucoseReading& reading, bool dataIsOld) const {
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    bool isFallingFast =
        (reading.trend == BG_TREND::DOUBLE_DOWN || reading.trend == BG_TREND::SINGLE_DOWN);

    // The expression follows the range, and a fall turns the in-range smile to shock. Each expression
    // swaps between its two frames. The color is the configured range color, so warning and urgent
    // differ and match the other faces.
    const bool firstFrame = millis() / 300 % 2 == 0;
    const uint8_t* faceBmp = firstFrame ? MOOD_HAPPY_A : MOOD_HAPPY_B;
    if (bgLevel == BG_LEVEL::URGENT_LOW || bgLevel == BG_LEVEL::WARNING_LOW || isFallingFast) {
        faceBmp = firstFrame ? MOOD_SHOCKED_A : MOOD_SHOCKED_B;
    }
    if (bgLevel == BG_LEVEL::URGENT_HIGH || bgLevel == BG_LEVEL::WARNING_HIGH) {
        faceBmp = firstFrame ? MOOD_DIZZY_A : MOOD_DIZZY_B;
    }
    uint16_t faceColor = getBandColor(bgLevel);

    if (dataIsOld) {
        faceColor = getDataOldColor();
    }

    DisplayManager.drawBitmap(0, 0, faceBmp, 8, 8, faceColor);

    String readingToDisplay = getPrintableReading(reading.sgv);
    uint8_t hueOffset = (millis() / 25) % 256;
    uint8_t textLen = readingToDisplay.length();
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    // Each character's width includes the one-pixel gap after it, which is not part of the drawn text.
    const int16_t textWidth = DisplayManager.getTextWidth(readingToDisplay.c_str(), 2) - 1;
    int16_t x = SMILEY_WIDTH + (ARROW_X - SMILEY_WIDTH - textWidth) / 2;
    for (uint8_t i = 0; i < textLen; i++) {
        char buf[2] = {readingToDisplay[i], '\0'};
        uint8_t charHue = hueOffset + (i * 35);
        uint16_t color = dataIsOld ? getDataOldColor() : getRainbowColor(charHue);
        DisplayManager.setTextColor(color);
        DisplayManager.printText(x, 6, buf, TEXT_ALIGNMENT::LEFT, 2);
        x += DisplayManager.getTextWidth(buf, 2);
    }
}
