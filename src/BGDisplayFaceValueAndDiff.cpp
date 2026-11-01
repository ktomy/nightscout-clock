#include "BGDisplayFaceValueAndDiff.h"

#include "BGDisplayValueText.h"
#include "globals.h"

void BGDisplayFaceValueAndDiff::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();

    // treating special casse where the value representation is too wide, so we move it one pixel to the
    // right

    if (SettingsManager.settings.bg_units == BG_UNIT::MMOLL && lastReading.sgv >= 180) {
        showReading(lastReading, 14, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    } else {
        showReading(lastReading, 13, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    }

    showTrendArrow(lastReading, 13, 1, dataIsOld);

    String diff = getDiff(readings);

    if (dataIsOld) {
        DisplayManager.setTextColor(getDataOldColor());
    } else {
        DisplayManager.setTextColor(COLOR_WHITE);
    }

    DisplayManager.printText(33, 6, diff.c_str(), TEXT_ALIGNMENT::RIGHT, 2);

    // Calculate time since last data update
    int elapsedMinutes = (ServerManager.getUtcEpoch() - lastReading.epoch) / 60;

    // Call timer block function
    drawTimerBlocks(lastReading, MATRIX_WIDTH, 0, 7);
}

String BGDisplayFaceValueAndDiff::getDiff(const std::list<GlucoseReading>& readings) const {
    const std::string diff = formatDiff(readings, SettingsManager.settings.bg_units != BG_UNIT::MGDL);

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF("SGV Diff: %s", diff.c_str());
#endif

    return String(diff.c_str());
}
