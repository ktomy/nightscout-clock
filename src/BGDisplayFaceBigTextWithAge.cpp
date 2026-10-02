#include "BGDisplayFaceBigTextWithAge.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigTextWithAge::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, dataIsOld);

    // Trend arrow moved up to y=0 (was y=1 on BigText) to leave row 7 free
    // for the age-indicator dots underneath.
    showTrendArrow(readings.back(), MATRIX_WIDTH - 5, 0, dataIsOld);

    // Age dots: one per minute since the last reading, under the arrow.
    drawTimerBlocks(readings.back(), 5, MATRIX_WIDTH - 5, 7);
}
