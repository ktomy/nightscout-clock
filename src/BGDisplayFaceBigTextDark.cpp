#include "BGDisplayFaceBigTextDark.h"

void BGDisplayFaceBigTextDark::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showDarkReading(readings.back(), dataIsOld, 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE);
}
