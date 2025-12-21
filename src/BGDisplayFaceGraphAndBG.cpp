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
    showTrendVerticalLine(31, lastReading.trend, dataIsOld);
    BGDisplayManager_::drawTimerBlocks(lastReading, textWidth + 2, graphWidth, 7);
}
