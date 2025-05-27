#include "BGDisplayFaceSimple.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceSimple::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    showReading(lastReading, 0, 6, TEXT_ALIGNMENT::CENTER, FONT_TYPE::MEDIUM, dataIsOld);

    // show arrow in the right part of the screen
    showTrendArrow(lastReading, MATRIX_WIDTH - 5, 1);

    // Call timer block function
    BGDisplayManager_::drawTimerBlocks(lastReading, MATRIX_WIDTH, 0, 7);
}
