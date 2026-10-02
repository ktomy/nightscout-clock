#include "BGDisplayFaceNyanCat.h"
#include "BGDisplayManager.h"
#include "globals.h"

namespace {
int16_t nyanCatScrollX = -12;
unsigned long nyanCatLastStepMs = 0, nyanCatPauseStartMs = 0;
bool nyanCatIsPaused = false;

// The cat scrolls in from off-screen; skip pixels outside the 32x8 panel
// instead of relying on uint8_t wraparound in drawPixel.
void nyanCatPixel(int16_t x, int16_t y, uint16_t color, bool updateMatrix) {
    if (x < 0 || x > 31 || y < 0 || y > 7) return;
    DisplayManager.drawPixel((uint8_t)x, (uint8_t)y, color, updateMatrix);
}

}

void BGDisplayFaceNyanCat::onActivate() const {
    nyanCatScrollX = -12;
    nyanCatLastStepMs = 0;
    nyanCatPauseStartMs = 0;
    nyanCatIsPaused = false;
}

bool BGDisplayFaceNyanCat::needsFrequentRefresh() const { return true; }

unsigned long BGDisplayFaceNyanCat::getFrequentRefreshIntervalMs() const { return 35; }

void BGDisplayFaceNyanCat::drawCat(int16_t x, int16_t y, uint8_t frame) const {
    uint16_t gray = DisplayManager.rgb565(170, 170, 170);
    uint16_t dkgray = DisplayManager.rgb565(100, 100, 100);
    uint16_t pink = DisplayManager.rgb565(255, 150, 200);
    uint16_t pawPink = DisplayManager.rgb565(255, 105, 180);
    // Tail (up, trailing left)
    nyanCatPixel(x + 0, y + 2, gray, false); nyanCatPixel(x + 0, y + 3, gray, false);
    nyanCatPixel(x + 0, y + 4, gray, false);
    // Cat body (gray)
    for (int8_t bx = 1; bx <= 4; bx++) {
        nyanCatPixel(x + bx, y + 2, gray, false); nyanCatPixel(x + bx, y + 3, gray, false);
        nyanCatPixel(x + bx, y + 4, gray, false); nyanCatPixel(x + bx, y + 5, gray, false);
    }
    // Cat head (gray with ears, leading right)
    nyanCatPixel(x + 5, y + 0, gray, false); nyanCatPixel(x + 9, y + 0, gray, false);
    nyanCatPixel(x + 5, y + 1, gray, false); nyanCatPixel(x + 6, y + 1, gray, false);
    nyanCatPixel(x + 8, y + 1, gray, false); nyanCatPixel(x + 9, y + 1, gray, false);
    nyanCatPixel(x + 5, y + 2, gray, false); nyanCatPixel(x + 6, y + 2, dkgray, false);
    nyanCatPixel(x + 7, y + 2, gray, false); nyanCatPixel(x + 8, y + 2, dkgray, false);
    nyanCatPixel(x + 9, y + 2, gray, false);
    nyanCatPixel(x + 5, y + 3, gray, false); nyanCatPixel(x + 6, y + 3, pink, false);
    nyanCatPixel(x + 7, y + 3, gray, false); nyanCatPixel(x + 8, y + 3, pink, false);
    nyanCatPixel(x + 9, y + 3, gray, false);
    nyanCatPixel(x + 6, y + 4, gray, false); nyanCatPixel(x + 7, y + 4, dkgray, false);
    nyanCatPixel(x + 8, y + 4, gray, false);
    // Legs (animated, like unicorn: gray with pink paws)
    if (frame == 0) {
        nyanCatPixel(x + 1, y + 6, gray, false); nyanCatPixel(x + 1, y + 7, pawPink, false);
        nyanCatPixel(x + 3, y + 6, gray, false); nyanCatPixel(x + 3, y + 7, pawPink, false);
    } else {
        nyanCatPixel(x + 2, y + 6, gray, false); nyanCatPixel(x + 2, y + 7, pawPink, false);
    }
}

void BGDisplayFaceNyanCat::drawNyanRainbow(int16_t startX, int16_t endX, uint8_t waveTick) const {
    if (endX < 0 || startX > 31) return;
    int16_t x0 = max((int16_t)0, startX), x1 = min((int16_t)31, endX);
    const uint16_t nyan[5] = { DisplayManager.rgb565(255, 0, 55), DisplayManager.rgb565(255, 140, 0), DisplayManager.rgb565(255, 235, 0), DisplayManager.rgb565(0, 255, 60), DisplayManager.rgb565(160, 40, 255) };
    for (int16_t x = x0; x <= x1; x++) {
        int seg = ((x / 2) + waveTick) % 2;
        int yBase = (seg == 0) ? 1 : 2;
        for (int b = 0; b < 5; b++) DisplayManager.drawPixel(x, yBase + b, nyan[b], false);
        if ((x + waveTick) % 5 == 0) DisplayManager.drawPixel(x, (seg == 0 ? 7 : 0), COLOR_WHITE, false);
    }
}

void BGDisplayFaceNyanCat::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    unsigned long now = millis();
    uint8_t leg = (now / 110) % 2, wave = (now / 110) % 4;
    auto lastReading = readings.back();
    String valStr = getPrintableReading(lastReading.sgv);
    int valWidth = DisplayManager.getTextWidth(valStr.c_str(), 2);

    if (nyanCatIsPaused) {
        if (now - nyanCatPauseStartMs > 3500) nyanCatIsPaused = false;
    } else if (now - nyanCatLastStepMs > 35) {
        nyanCatScrollX++;
        if (nyanCatScrollX == 24) { nyanCatIsPaused = true; nyanCatPauseStartMs = now; }
        if (nyanCatScrollX > (32 + valWidth + 18)) nyanCatScrollX = -12;
        nyanCatLastStepMs = now;
    }

    int16_t ux = nyanCatScrollX, tEnd = ux + 1, tStart = ux - 10, bgX = tStart - valWidth - 2, arrX = bgX + valWidth + 1;
    drawNyanRainbow(tStart, tEnd, wave);
    drawCat(ux, 0, leg);

    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    uint8_t hue = (now / 15) % 255;
    int16_t curX = bgX;
    for (size_t i = 0; i < valStr.length(); i++) {
        char buf[2] = {valStr[i], '\0'};
        uint16_t col = dataIsOld ? getDataOldColor() : DisplayManager.hsvToRgb565(hue + (i * 35));
        DisplayManager.setTextColor(col);
        DisplayManager.printText(curX, 6, buf, TEXT_ALIGNMENT::LEFT, 2);
        curX += DisplayManager.getTextWidth(buf, 2);
    }
    showTrendArrow(lastReading, arrX, 1, dataIsOld);
}
