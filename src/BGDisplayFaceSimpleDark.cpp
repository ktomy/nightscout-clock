#include "BGDisplayFaceSimpleDark.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceSimpleDark::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    const auto& reading = readings.back();
    const uint16_t bandColor = getColorByBGValue(reading);
    const auto level = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    // An urgent number takes the urgent color, so it stays obvious even without a trend arrow.
    const bool urgent = level == BG_LEVEL::URGENT_LOW || level == BG_LEVEL::URGENT_HIGH;
    const uint16_t valueColor =
        urgent ? bandColor
               : static_cast<uint16_t>(SettingsManager.settings.face_simple_dark.value_color);
    DisplayManager.setTextColor(dataIsOld ? getDataOldColor() : valueColor);
    printReading(reading, 0, 6, TEXT_ALIGNMENT::CENTER, FONT_TYPE::MEDIUM);

    showTrendArrow(reading, MATRIX_WIDTH - 5, 1, dataIsOld, bandColor);
}
