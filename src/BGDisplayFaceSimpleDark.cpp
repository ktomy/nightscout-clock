#include "BGDisplayFaceSimpleDark.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceSimpleDark::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showDarkReading(readings.back(), dataIsOld, 0, 6, TEXT_ALIGNMENT::CENTER, FONT_TYPE::MEDIUM);
}

void BGDisplayFaceSimpleDark::showDarkReading(
    const GlucoseReading& reading, bool dataIsOld, int16_t x, int16_t y, TEXT_ALIGNMENT alignment,
    FONT_TYPE font) const {
    const uint16_t bandColor = getColorByBGValue(reading);
    const auto level = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    // An urgent number takes the urgent color, so it stays obvious even without a trend arrow.
    const bool urgent = level == BG_LEVEL::URGENT_LOW || level == BG_LEVEL::URGENT_HIGH;
    const uint16_t valueColor =
        urgent ? bandColor
               : static_cast<uint16_t>(SettingsManager.settings.face_simple_dark.value_color);
    DisplayManager.setTextColor(dataIsOld ? getDataOldColor() : valueColor);
    printReading(reading, x, y, alignment, font);

    showTrendArrow(reading, MATRIX_WIDTH - 5, 1, dataIsOld, bandColor);
}
