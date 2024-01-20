#include "BGDisplayFaceBigText.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigText::showReadings(const std::list<GlucoseReading> &readings) const {

    DisplayManager.clearMatrix();

    showReading(readings.back(), 0, 6, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE);

    // show arrow in the right part of the screen
    showTrendArrow(readings.back(), 32 - 5, 1);
}

void BGDisplayFaceBigText::markDataAsOld() const {}
