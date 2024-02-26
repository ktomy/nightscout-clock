#include "BGDisplayFaceValueAndDiff.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceValueAndDiff::showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld) const {

    DisplayManager.clearMatrix();

    auto lastReading = readings.back();

    // treating special casse where the value representation is too wide, so we move it one pixel to the right

    if (SettingsManager.settings.bg_units == BG_UNIT::MMOLL && lastReading.sgv >= 180) {
        showReading(lastReading, 14, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    } else {
        showReading(lastReading, 13, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    }

    showTrendArrow(lastReading, 13, 1);

    String diff = getDiff(readings);

    if (dataIsOld) {
        DisplayManager.setTextColor(BG_COLOR_OLD);
    } else {
        DisplayManager.setTextColor(COLOR_WHITE);
    }
#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF("Diff: %s", diff.c_str());
#endif

    DisplayManager.printText(33, 6, diff.c_str(), TEXT_ALIGNMENT::RIGHT, 2);
}

String BGDisplayFaceValueAndDiff::getDiff(const std::list<GlucoseReading> &readings) const {
    if (readings.size() < 2) {
        return "";
    }

    auto last = readings.back();
    auto prev = std::prev(readings.end(), 2);
    if (last.getSecondsAgo() - prev->getSecondsAgo() > 6 * 60) {
        return "";
    }

    int diff = last.sgv - prev->sgv;

    if (std::abs(diff) > 99) {
        return "?";
    }

    String diffString = "";
    if (diff > 0) {
        diffString += "+";
    }

    diffString += getPrintableReading(diff);
    return diffString;
}
