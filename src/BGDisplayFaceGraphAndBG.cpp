#include "BGDisplayFaceGraphAndBG.h"
#include "BGDisplayManager.h"
#include "enums.h"
#include "globals.h"
#include <Arduino.h>

void BGDisplayFaceGraphAndBG::showReadings(const std::list<GlucoseReading> &readings) const {

    GlucoseReading reading = readings.back();
    String printableReading = getPrintableReading(reading);
    uint8_t textWidth = DisplayManager.getTextWidth(printableReading.c_str(), 2);
    // 32 is the width of the display, 2 for arrow and space
    uint8_t grqphWidth = 32 - textWidth - 2;
    uint8_t minutesToShow = grqphWidth * 5;

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF("For the value %d, printable is: %s, text width: %u, graph width: %u\n", reading.sgv, printableReading.c_str(),
                 textWidth, grqphWidth);
#endif

    showGraph(0, grqphWidth, minutesToShow, readings);
    showReading(readings.back(), 31, 6, RIGHT);
    showTrendVerticalLine(31, readings.back().trend);
}

void BGDisplayFaceGraphAndBG::markDataAsOld() const {}

void BGDisplayFaceGraphAndBG::showTrendVerticalLine(int x, BG_TREND trend) const {
    switch (trend) {
        case DoubleUp:
            DisplayManager.drawPixel(x, 0, COLOR_RED);
            DisplayManager.drawPixel(x, 1, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case DoubleDown:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            DisplayManager.drawPixel(x, 6, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 7, COLOR_RED);
            break;
        case SingleUp:
            DisplayManager.drawPixel(x, 1, COLOR_YELLOW);
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case SingleDown:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            DisplayManager.drawPixel(x, 6, COLOR_YELLOW);
            break;
        case FortyFiveUp:
            DisplayManager.drawPixel(x, 2, COLOR_GREEN);
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            break;
        case FortyFiveDown:
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            DisplayManager.drawPixel(x, 5, COLOR_GREEN);
            break;
        case Flat:
            DisplayManager.drawPixel(x, 3, COLOR_WHITE);
            DisplayManager.drawPixel(x, 4, COLOR_WHITE);
            break;
        default:
            DisplayManager.setTextColor(COLOR_BLACK);
            break;
    }
    DisplayManager.update();
}
