#include "BGDisplayFaceClock.h"

#include "BGDisplayManager.h"
#include "ServerManager.h"
#include "globals.h"

void BGDisplayFaceClock::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showClock();

    showReading(readings.back(), 33, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);

    switch (SettingsManager.settings.time_format) {
        case TIME_FORMAT::HOURS_12:
            BGDisplayManager_::drawTimerBlocks(readings.back(), MATRIX_WIDTH - 17, 18, 7);
            break;
        default:
            BGDisplayManager_::drawTimerBlocks(readings.back(), MATRIX_WIDTH, 0, 7);
            break;
    }

    showTrendVerticalLine(17, readings.back().trend, dataIsOld);
}

void BGDisplayFaceClock::showClock() const {
    // Show current time

    tm timeinfo = ServerManager.getTimezonedTime();

    auto time_format = SettingsManager.settings.time_format;
    if (time_format == TIME_FORMAT::HOURS_12) {
        bool is_pm = timeinfo.tm_hour >= 12;
        if (timeinfo.tm_hour == 0) {
            timeinfo.tm_hour = 12;
        } else if (timeinfo.tm_hour > 12) {
            timeinfo.tm_hour -= 12;
        }

        for (int i = 0; i < 16; i++) {
            DisplayManager.drawPixel(i, 7, is_pm ? COLOR_BLUE : COLOR_CYAN);
        }
    }

    char hour[3], minute[3];
    snprintf(hour, sizeof(hour), "%02d", timeinfo.tm_hour);
    snprintf(minute, sizeof(minute), "%02d", timeinfo.tm_min);

    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.printText(0, 6, hour, TEXT_ALIGNMENT::LEFT, 2);
    DisplayManager.printText(9, 6, minute, TEXT_ALIGNMENT::LEFT, 2);
}

void BGDisplayFaceClock::showNoData() const {
    DisplayManager.clearMatrix();
    showClock();

    String noData = "---";
    if (SettingsManager.settings.bg_units == BG_UNIT::MMOLL) {
        noData = "--.-";
    }

    DisplayManager.setTextColor(BG_COLOR_OLD);
    DisplayManager.printText(33, 6, noData.c_str(), TEXT_ALIGNMENT::RIGHT, 2);
}
