#include "BGDisplayFaceBigTextRainbow.h"

#include "globals.h"

namespace {
constexpr unsigned long RAINBOW_STEP_MS = 80;
}

// Old data shows still in the old-data color; fresh data is drawn by the animation frame.
void BGDisplayFaceBigTextRainbow::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    if (!dataIsOld) {
        return;
    }
    showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, true);
    showTrendArrow(readings.back(), MATRIX_WIDTH - 5, 1, true);
}

unsigned long BGDisplayFaceBigTextRainbow::getAnimationStepMillis() const { return RAINBOW_STEP_MS; }

void BGDisplayFaceBigTextRainbow::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    const GlucoseReading& lastReading = readings.back();
    DisplayManager.clearMatrix();
    showAnimatedReading(lastReading);
    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1, false, getColorByBGValue(lastReading));
}

// Each character takes the next hue of a moving rainbow whatever the value; the arrow shows the range.
void BGDisplayFaceBigTextRainbow::showAnimatedReading(const GlucoseReading& reading) const {
    const String readingToDisplay = getPrintableReading(reading.sgv);
    DisplayManager.setFont(FONT_TYPE::LARGE);

    const uint8_t hueOffset = (millis() / 20) % 256;
    const uint8_t textLength = readingToDisplay.length();
    int16_t x = 0;
    for (uint8_t i = 0; i < textLength; i++) {
        char text[2] = {readingToDisplay[i], '\0'};
        const uint8_t hue = map(i, 0, max((int)textLength, 1), 0, 255) + hueOffset;
        DisplayManager.setTextColor(getRainbowColor(hue));
        DisplayManager.printText(x, 7, text, TEXT_ALIGNMENT::LEFT, 2);
        x += DisplayManager.getTextWidth(text, 2);
    }
}
