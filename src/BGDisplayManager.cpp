#include "BGDisplayManager.h"
#include "globals.h"
#include "NightscoutManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"

#include <list>

// The getter for the instantiated singleton instance
BGDisplayManager_ &BGDisplayManager_::getInstance()
{
    static BGDisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGDisplayManager_ &BGDisplayManager = BGDisplayManager.getInstance();

void BGDisplayManager_::setup() {

}

void BGDisplayManager_::tick() {
    ///TODO: Move back the glucose graph
    ///TODO: Check if the last reading is too ald and make it gray

}

void BGDisplayManager_::showGlucoseGraph(std::list<GlucoseReading> readings) {
    if (readings.size() == 0) {
        return;
    }
}

void BGDisplayManager_::showReading(GlucoseReading reading) {

    char trendSymbol = 0xA1;
    // if (reading.trend > 0 && reading.trend < 8) {
    //     trendSymbol = (char)(reading.trend + 0xA0);
    // }

    String readingToDisplay = "";
    if (SettingsManager.settings.bgUnit == MGDL) {
        readingToDisplay += String(reading.sgv) + String(trendSymbol);
    } else {
        char buffer[10];
        sprintf(buffer, "%.1f", round((float)reading.sgv / 1.8) / 10);
        readingToDisplay += String(buffer) + String(trendSymbol);
    }

    DisplayManager.clearMatrix();
    DisplayManager.printText(0, 6, readingToDisplay.c_str(), true, 2);
}


void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings) {

    if (glucoseReadings.size() == 0) {
        return;
    }

    showGlucoseGraph(glucoseReadings);
    showReading(glucoseReadings.back());
    displayedReadings = glucoseReadings;
}

unsigned long long BGDisplayManager_::getLastDisplayedGlucoseEpoch() {
    if (displayedReadings.size() > 0) {
        return displayedReadings.back().epoch;
    } else {
        return 0;
    }
}
