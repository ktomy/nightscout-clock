#include "BGDisplayFaceGraph.h"
#include "BGDisplayManager.h"
#include <Arduino.h>
#include "globals.h"

void BGDisplayFaceGraph::showReadings(const std::list<GlucoseReading>& readings) const {
    showReading(readings.back());
    showGraph(readings);
}

void BGDisplayFaceGraph::markDataAsOld() const {
    
}

void BGDisplayFaceGraph::showReading(const GlucoseReading reading) const {
}

int getAverageValueForPeriod(uint16_t fromSecondsAgo, uint16_t toSecondsAgo, std::list<GlucoseReading> readings)
{
    unsigned long long fromEpoch = time(NULL) - fromSecondsAgo;
    unsigned long long toEpoch = time(NULL) - toSecondsAgo;
    // note, toEpoch is older than fromEpoch

    long sum = 0;
    int count = 0;
    for (const GlucoseReading& reading : readings) {
        if (reading.epoch >= toEpoch && reading.epoch < fromEpoch) {
            sum += reading.sgv;
            count++;
        }
    }

    int average = -1;

    if (count > 0) {
        average = sum / count;
    }

    DEBUG_PRINTF("Calculating average between %d (%llu) and %d (%llu). Values found: %d, sum is %ld. Average is %d\n" ,fromSecondsAgo, fromEpoch, toSecondsAgo, toEpoch, count, sum, average);

    return average;
}


void showGraphInternal(uint8_t x_position, uint8_t length, uint16_t forMinutes, const std::list<GlucoseReading>& readings)
{
    auto pixelSize = forMinutes / length;
    String pixels = "Graph pixels: ";
    DEBUG_PRINTF("Pixel size: %d\n", pixelSize);
    String readingsDebug = "Readings: ";
    for (const GlucoseReading& reading : readings) {
        readingsDebug += reading.toString() + " | ";
    }

    DEBUG_PRINTLN(readingsDebug);

    auto intervals = BGDisplayManager.getGlucoseIntervals();
    DEBUG_PRINTLN(intervals.toString());
    GlucoseInterval normalInterval = GlucoseInterval();;
    for (const GlucoseInterval& interval : intervals.intervals) {
        if (interval.intarval_type == NORMAL) {
            normalInterval = interval;
            break;
        }
    }

    if (normalInterval.intarval_type == INVALID) {
        DisplayManager.showFatalError("No normal interval present, please set up");
    }

    for(int i = 0; i < length; i++)
    {
        auto average = getAverageValueForPeriod(i * pixelSize * 60, (i+1) * pixelSize * 60, readings);

        if (average < 0) {
            pixels += "_ ";
            continue;
        }

        int y = -1;
        uint16_t color;

        switch(intervals.getBGLevel(average)) {
            case URGENT_HIGH:
                y = 0;
                color = 0xF800;
                break;
            case WARNING_HIGH:
                y = 1;
                color = 0xFFE0;
                break;
            case NORMAL:
                y = (average - normalInterval.low_boundary) * 4 / (normalInterval.high_boundary - normalInterval.low_boundary);
                color = 0x07E0;
                break;
            case WARNING_LOW:
                y = 6;
                color = 0xFFE0;
                break;
            case URGENT_LOW:
                y = 7;
                color = 0xF800;
                break;
            default:
                y = 0;
                color = 0xa514;
                break;
        }

        pixels += String(y) + "(" + String(average) + ")" + " ";

        DisplayManager.drawPixel(x_position + length - 1 - i, y, color);
    }

    DEBUG_PRINTLN(pixels);

}


void BGDisplayFaceGraph::showGraph(const std::list<GlucoseReading>& readings) const
{
    showGraphInternal(0, 32, 180, readings);
}