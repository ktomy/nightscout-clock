#include "BGDisplayFaceBigTextWithAge.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigTextWithAge::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, dataIsOld);

    // Trend arrow moved up to y=0 (was y=1 on BigText) to leave row 7 free
    // for the age-indicator dots underneath.
    showTrendArrow(readings.back(), MATRIX_WIDTH - 5, 0, dataIsOld);

    // Age dots: one single-pixel dot per minute since the last reading,
    // underneath the arrow. Width 9 gives blockSize 1 (single-pixel dots);
    // x = MATRIX_WIDTH-9 keeps all 5 dots on screen (23,25,27,29,31).
    drawTimerBlocks(readings.back(), 9, MATRIX_WIDTH - 9, 7);
}
