#include "BGDisplayFaceGraphAndBG.h"
#include "BGDisplayManager.h"
#include "globals.h"
#include <Arduino.h>

void BGDisplayFaceGraphAndBG::showReadings(const std::list<GlucoseReading> &readings) const {
    showGraphBase(0, 16, 180, readings);
}

void BGDisplayFaceGraphAndBG::markDataAsOld() const {}

void BGDisplayFaceGraphAndBG::showLastReading(const GlucoseReading &reading) const {}
