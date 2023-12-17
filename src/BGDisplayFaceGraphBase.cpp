#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayManager.h"
#include "globals.h"
#include <Arduino.h>

int getAverageValueForPeriod(uint16_t fromSecondsAgo, uint16_t toSecondsAgo, std::list<GlucoseReading> readings) {
    unsigned long long fromEpoch = time(NULL) - fromSecondsAgo;
    unsigned long long toEpoch = time(NULL) - toSecondsAgo;
    // note, toEpoch is older than fromEpoch

    long sum = 0;
    int count = 0;
    for (const GlucoseReading &reading : readings) {
        if (reading.epoch >= toEpoch && reading.epoch < fromEpoch) {
            sum += reading.sgv;
            count++;
        }
    }

    int average = -1;

    if (count > 0) {
        average = sum / count;
    }

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF("Calculating average between %d (%llu) and %d (%llu). Values found: %d, sum is %ld. Average is %d\n",
                 fromSecondsAgo, fromEpoch, toSecondsAgo, toEpoch, count, sum, average);
#endif

    return average;
}

int getNormalIntervalYPosition(int value, GlucoseInterval interval) {
    int yForHighestValue = 2;
    int yForLowestValue = 5;

    int out_max = yForLowestValue + 1 - yForHighestValue;
    int relative_y_inverted = map(value, interval.low_boundary, interval.high_boundary, 0, out_max);
    int y = yForLowestValue - relative_y_inverted;

    return y;
}

void BGDisplayFaceGraphBase::showGraph(uint8_t x_position, uint8_t length, uint16_t forMinutes,
                                       const std::list<GlucoseReading> &readings) const {
    DisplayManager.clearMatrixPart(x_position, 0, length, 8);
    auto pixelSizeSeconds = (forMinutes * 60) / length;

    auto intervals = BGDisplayManager.getGlucoseIntervals();

#ifdef DEBUG_DISPLAY
    String pixels = "Graph pixels: ";
    DEBUG_PRINTF("Pixel size: %d seconds\n", pixelSizeSeconds);

    String readingsDebug = "Readings: ";
    for (const GlucoseReading &reading : readings) {
        readingsDebug += reading.toString() + " | ";
    }

    DEBUG_PRINTLN(readingsDebug);
    DEBUG_PRINTLN(intervals.toString());
#endif

    GlucoseInterval normalInterval = GlucoseInterval();

    for (const GlucoseInterval &interval : intervals.intervals) {
        if (interval.intarval_type == NORMAL) {
            normalInterval = interval;
            break;
        }
    }

    if (normalInterval.intarval_type == INVALID) {
        DisplayManager.showFatalError("No normal interval present, please set up");
    }

    for (int i = 0; i < length; i++) {
        auto average = getAverageValueForPeriod(i * pixelSizeSeconds, (i + 1) * pixelSizeSeconds, readings);

        if (average < 0) {
            pixels += "_ ";
            continue;
        }

        int y = -1;
        uint16_t color;

        switch (intervals.getBGLevel(average)) {
            case URGENT_HIGH:
                y = 0;
                color = COLOR_RED;
                break;
            case WARNING_HIGH:
                y = 1;
                color = COLOR_YELLOW;
                break;
            case NORMAL:
                y = getNormalIntervalYPosition(average, normalInterval);
                color = COLOR_GREEN;
                break;
            case WARNING_LOW:
                y = 6;
                color = COLOR_YELLOW;
                break;
            case URGENT_LOW:
                y = 7;
                color = COLOR_RED;
                break;
            default:
                y = 0;
                color = COLOR_GRAY;
                break;
        }

        pixels += String(y) + "(" + String(average) + ")" + " ";

        DisplayManager.drawPixel(x_position + length - 1 - i, y, color);
    }

    DisplayManager.update();

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTLN(pixels);
#endif
}
