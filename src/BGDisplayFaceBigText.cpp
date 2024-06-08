#include "BGDisplayFaceBigText.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigText::showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld) const {

    DisplayManager.clearMatrix();

    showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, dataIsOld);

    // show arrow in the right part of the screen
    showTrendArrow(readings.back(), 32 - 5, 1);
}
