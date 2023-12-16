#include "BGDisplayFaceGraphAndBG.h"
#include "BGDisplayManager.h"
#include "globals.h"
#include <Arduino.h>

void BGDisplayFaceGraphAndBG::showReadings(const std::list<GlucoseReading> &readings) const {

    DisplayManager.clearMatrix();

    showGraphBase(0, 16, 180, readings);
    showReadingBase(readings.back(), 16, 6, false);
}

void BGDisplayFaceGraphAndBG::markDataAsOld() const {}
