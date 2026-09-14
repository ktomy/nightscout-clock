#include "BGDisplayFaceSimpleDark.h"

#include "BGDisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"

void BGDisplayFaceSimpleDark::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();

    // Stale takes the whole face. Cyan rather than the data-old colour: gray is a legal
    // choice there and invisible at the brightness this face is used at.
    showReadingInColor(
        lastReading, 0, 6, TEXT_ALIGNMENT::CENTER, FONT_TYPE::MEDIUM,
        dataIsOld ? COLOR_CYAN : getValueColor(lastReading));
    showTrendArrowInColor(
        lastReading, MATRIX_WIDTH - 5, 1, dataIsOld ? COLOR_CYAN : getColorByBGValue(lastReading));
}

void BGDisplayFaceSimpleDark::showNoData() const {
    DisplayManager.clearMatrix();
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    DisplayManager.setTextColor(
        static_cast<uint16_t>(SettingsManager.settings.face_simple_dark.in_range_color));
    DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
}

uint16_t BGDisplayFaceSimpleDark::getValueColor(const GlucoseReading& reading) const {
    const auto& colors = SettingsManager.settings.face_simple_dark;
    DISPLAY_COLOR color = colors.in_range_color;
    switch (bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv)) {
        case BG_LEVEL::URGENT_LOW:
            color = colors.urgent_low_color;
            break;
        case BG_LEVEL::WARNING_LOW:
            color = colors.low_color;
            break;
        case BG_LEVEL::WARNING_HIGH:
            color = colors.high_color;
            break;
        case BG_LEVEL::URGENT_HIGH:
            color = colors.urgent_high_color;
            break;
        default:
            break;
    }
    return static_cast<uint16_t>(color);
}
