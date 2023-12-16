#include "BGDisplayFaceSimple.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceSimple::showReadings(const std::list<GlucoseReading> &readings) const {

    DisplayManager.clearMatrix();

    showReadingBase(readings.back(), 0, 6, true);

    // show arrow in the right part of the screen
    showTrendArrow(readings.back(), 32 - 5, 1);
}

void BGDisplayFaceSimple::markDataAsOld() const {}


