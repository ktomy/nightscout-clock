#include "BGDisplayFaceValueAndDiff.h"

#include "BGDisplayManager.h"
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
        DisplayManager.setTextColor(BG_COLOR_OLD);
    } else {
        DisplayManager.setTextColor(COLOR_WHITE);
    }

    DisplayManager.printText(33, 6, diff.c_str(), TEXT_ALIGNMENT::RIGHT, 2);

    // Calculate time since last data update
    int elapsedMinutes = (ServerManager.getUtcEpoch() - lastReading.epoch) / 60;

    // Call timer block function
    BGDisplayManager_::drawTimerBlocks(lastReading, MATRIX_WIDTH, 0, 7);
}

String BGDisplayFaceValueAndDiff::getDiff(const std::list<GlucoseReading>& readings) const {
    if (readings.size() < 2) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Not enough readings to calculate diff");
#endif
        return "";
    }

    auto last = readings.back();
    std::list<GlucoseReading> foundReadings;
    // cycle through reading starting from the last one until the time of reading is less than 6.5
    // minutes prior to the last reading. Store the found readings
    for (auto it = readings.rbegin(); it != readings.rend(); ++it) {
        if (last.epoch - it->epoch > (6 * 60 + 30)) {
            break;
        }
        foundReadings.push_back(*it);
    }

    // If we have a lot of readings, we are probably on Libre, so we just compare 2 last readings
    if (foundReadings.size() > 5) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Too many readings found, using only last 2");
#endif
        // truncate the list to keep only the last 2 readings
        foundReadings.resize(2);
    }

    // Cycle through found readings, get min and max SGVs.
    // sgv here is always raw mg/dL - these noise/sanity checks (below) intentionally
    // operate on the raw value regardless of the user's display unit, so switching
    // between mg/dL and mmol/L never changes which readings are considered valid.
    int minSGV = 9999;
    int maxSGV = 0;
    for (const auto& reading : foundReadings) {
        if (reading.sgv < minSGV) {
            minSGV = reading.sgv;
        }
        if (reading.sgv > maxSGV) {
            maxSGV = reading.sgv;
        }
    }
    int base = last.sgv;
    if (minSGV != base && maxSGV != base) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Too many changes in last 6.5 minutes");
#endif
        return "?";
    }

    int diff = minSGV == base ? base - maxSGV : base - minSGV;

    // Threshold stays in raw mg/dL (99, unchanged) - no unit conversion needed here.
    if (std::abs(diff) > 99) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Diff is too big: " + String(diff));
#endif
        return "?";
    }

    // Render the delta from the *displayed* value of each endpoint, rather than
    // converting the raw mg/dL diff directly. Two readings that differ in mg/dL can
    // still round to the same displayed mmol/L value (e.g. 193 and 192 mg/dL both show
    // as "10.7"): converting the raw diff directly would then print a false -0.1/+0.1
    // delta even though nothing changed on screen. Diffing the two rounded display
    // values instead guarantees the delta always matches what the user can see.
    //
    // Trade-off: for larger gaps this can differ from "true rate of change, rounded
    // once" by up to 0.1 mmol/L, since each endpoint is rounded independently before
    // subtracting. That is intentional - it keeps the delta consistent with the two
    // numbers on screen, which is what was actually reported as confusing.
    int otherSGV = minSGV == base ? maxSGV : minSGV;
    int diffDisplayTenths = toDisplayTenths(base) - toDisplayTenths(otherSGV);

    String diffString = "";
    if (diffDisplayTenths >= 0) {
        diffString += "+";
    }

    diffString += formatDisplayTenths(diffDisplayTenths);

#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF("SGV Diff: %s (%d readings)", diffString.c_str(), foundReadings.size());
#endif

    return diffString;
}
