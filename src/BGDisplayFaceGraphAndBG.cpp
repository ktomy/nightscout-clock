#include "BGDisplayFaceGraphAndBG.h"

#include <Arduino.h>

#include "BGDisplayManager.h"
#include "enums.h"
#include "globals.h"

void BGDisplayFaceGraphAndBG::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    GlucoseReading reading = readings.back();
    String printableReading = getPrintableReading(reading.sgv);
    uint8_t textWidth = DisplayManager.getTextWidth(printableReading.c_str(), 2);
    // 2 for arrow and space
    uint8_t graphWidth = MATRIX_WIDTH - textWidth - 2;
    uint8_t minutesToShow = graphWidth * 5;

    auto lastReading = readings.back();

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF(
        "For the value %d, printable is: %s, text width: %u, graph width: %u\n", reading.sgv,
        printableReading.c_str(), textWidth, graphWidth);
#endif

    showGraph(0, graphWidth, minutesToShow, readings);
    showReading(lastReading, 31, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    showTrendVerticalLine(31, lastReading.trend);
    BGDisplayManager_::drawTimerBlocks(lastReading, textWidth + 2, graphWidth, 7);
}

void BGDisplayFaceGraphAndBG::showTrendVerticalLine(int x, BG_TREND trend) const {
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
