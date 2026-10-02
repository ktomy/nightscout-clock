#include "BGDisplayFaceNyanUnicorn.h"
#include "globals.h"

// hsvToRgb is defined in DisplayManager.cpp (returns an RGB565-packed value);
// upstream's DisplayManager has no rgb565 helper, so pack locally.
uint32_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v);
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

namespace {
int16_t nyanScrollX = -12;
unsigned long nyanLastStepMs = 0, nyanPauseStartMs = 0;
bool nyanIsPaused = false;

// The unicorn scrolls in from off-screen; skip pixels outside the 32x8 panel
// instead of relying on uint8_t wraparound in drawPixel.
void nyanPixel(int16_t x, int16_t y, uint16_t color, bool updateMatrix) {
    if (x < 0 || x > 31 || y < 0 || y > 7) return;
    DisplayManager.drawPixel((uint8_t)x, (uint8_t)y, color, updateMatrix);
}

}

void BGDisplayFaceNyanUnicorn::onActivate() const {
    nyanScrollX = -12;
    nyanLastStepMs = 0;
    nyanPauseStartMs = 0;
    nyanIsPaused = false;
}

bool BGDisplayFaceNyanUnicorn::needsFrequentRefresh() const { return true; }

unsigned long BGDisplayFaceNyanUnicorn::getFrequentRefreshIntervalMs() const { return 35; }

void BGDisplayFaceNyanUnicorn::drawUnicorn(int16_t x, int16_t y, uint8_t frame) const {
    nyanPixel(x + 6, y + 0, rgb565(255, 215, 0), false);
    nyanPixel(x + 5, y + 1, rgb565(255, 215, 0), false);
    nyanPixel(x + 4, y + 1, rgb565(255, 20, 147), false);
    nyanPixel(x + 3, y + 2, rgb565(255, 105, 180), false);
    nyanPixel(x + 4, y + 2, COLOR_WHITE, false);
    nyanPixel(x + 6, y + 2, COLOR_WHITE, false);
    nyanPixel(x + 6, y + 3, rgb565(255, 182, 193), false);
    nyanPixel(x + 5, y + 2, rgb565(0, 191, 255), false);
    nyanPixel(x + 3, y + 3, COLOR_WHITE, false);
    nyanPixel(x + 4, y + 3, COLOR_WHITE, false);
    nyanPixel(x + 5, y + 3, COLOR_WHITE, false);
    for (int8_t bx = 2; bx <= 5; bx++) {
        nyanPixel(x + bx, y + 4, COLOR_WHITE, false);
        nyanPixel(x + bx, y + 5, COLOR_WHITE, false);
    }
    uint16_t hoof = rgb565(255, 105, 180);
    if (frame == 0) {
        nyanPixel(x + 2, y + 6, COLOR_WHITE, false); nyanPixel(x + 1, y + 7, hoof, false);
        nyanPixel(x + 5, y + 6, COLOR_WHITE, false); nyanPixel(x + 6, y + 7, hoof, false);
    } else {
        nyanPixel(x + 3, y + 6, COLOR_WHITE, false); nyanPixel(x + 2, y + 7, hoof, false);
        nyanPixel(x + 4, y + 6, COLOR_WHITE, false); nyanPixel(x + 5, y + 7, hoof, false);
    }
}

void BGDisplayFaceNyanUnicorn::drawNyanRainbow(int16_t startX, int16_t endX, uint8_t waveTick) const {
    if (endX < 0 || startX > 31) return;
    int16_t x0 = max((int16_t)0, startX), x1 = min((int16_t)31, endX);
    const uint16_t nyan[5] = { rgb565(255, 0, 55), rgb565(255, 140, 0), rgb565(255, 235, 0), rgb565(0, 255, 60), rgb565(160, 40, 255) };
    for (int16_t x = x0; x <= x1; x++) {
        int seg = ((x / 2) + waveTick) % 2;
        int yBase = (seg == 0) ? 1 : 2;
        for (int b = 0; b < 5; b++) DisplayManager.drawPixel(x, yBase + b, nyan[b], false);
        if ((x + waveTick) % 5 == 0) DisplayManager.drawPixel(x, (seg == 0 ? 7 : 0), COLOR_WHITE, false);
    }
}

void BGDisplayFaceNyanUnicorn::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    unsigned long now = millis();
    uint8_t leg = (now / 110) % 2, wave = (now / 110) % 4;
    auto lastReading = readings.back();
    String valStr = getPrintableReading(lastReading.sgv);
    int valWidth = DisplayManager.getTextWidth(valStr.c_str(), 2);

    if (nyanIsPaused) {
        if (now - nyanPauseStartMs > 3500) nyanIsPaused = false;
    } else if (now - nyanLastStepMs > 35) {
        nyanScrollX++;
        if (nyanScrollX == 24) { nyanIsPaused = true; nyanPauseStartMs = now; }
        if (nyanScrollX > (32 + valWidth + 18)) nyanScrollX = -12;
        nyanLastStepMs = now;
    }

    int16_t ux = nyanScrollX, tEnd = ux + 1, tStart = ux - 10, bgX = tStart - valWidth - 2, arrX = bgX + valWidth + 1;
    drawNyanRainbow(tStart, tEnd, wave);
    drawUnicorn(ux, 0, leg);

    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    uint8_t hue = (now / 15) % 255;
    int16_t curX = bgX;
    for (size_t i = 0; i < valStr.length(); i++) {
        char buf[2] = {valStr[i], '\0'};
        uint16_t col = dataIsOld ? getDataOldColor() : (uint16_t)hsvToRgb(hue + (i * 35), 255, 255);
        DisplayManager.setTextColor(col);
        DisplayManager.printText(curX, 6, buf, TEXT_ALIGNMENT::LEFT, 2);
        curX += DisplayManager.getTextWidth(buf, 2);
    }
    showTrendArrow(lastReading, arrX, 1, dataIsOld);
}
