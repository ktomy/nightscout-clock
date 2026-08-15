#include "BGDisplayFaceClock.h"

#include "ServerManager.h"
#include "globals.h"

void BGDisplayFaceClock::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showClock();

    showReading(readings.back(), 31, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);

    switch (SettingsManager.settings.time_format) {
        case TIME_FORMAT::HOURS_12:
            drawTimerBlocks(readings.back(), MATRIX_WIDTH - 17, 18, 7);
            break;
        default:
            drawTimerBlocks(readings.back(), MATRIX_WIDTH, 0, 7);
            break;
    }

    showTrendVerticalLine(31, readings.back().trend, dataIsOld);
}

RenderDecision BGDisplayFaceClock::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason == RenderReason::TIME_TICK && ctx.readings.empty()) {
        return RenderDecision::PARTIAL;
    }

    return BGDisplayFaceWithAge::getRenderDecision(ctx);
}

RenderDecision BGDisplayFaceClock::getAgeTickRenderDecision() const { return RenderDecision::PARTIAL; }

void BGDisplayFaceClock::renderPartial(const RenderContext& ctx) const {
    clearClockRegion(!ctx.readings.empty());
    showClock();

    if (!ctx.readings.empty()) {
        switch (SettingsManager.settings.time_format) {
            case TIME_FORMAT::HOURS_12:
                drawTimerBlocks(ctx.readings.back(), MATRIX_WIDTH - 17, 18, 7);
                break;
            default:
                drawTimerBlocks(ctx.readings.back(), MATRIX_WIDTH, 0, 7);
                break;
        }
    }
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

void BGDisplayFaceClock::clearClockRegion(bool hasTimerBlocks) const {
    DisplayManager.clearMatrixPart(0, 0, 16, 7);

    switch (SettingsManager.settings.time_format) {
        case TIME_FORMAT::HOURS_12:
            DisplayManager.clearMatrixPart(0, 7, 16, 1);
            if (hasTimerBlocks) {
                DisplayManager.clearMatrixPart(18, 7, 13, 1);
            }
            break;
        default:
            if (hasTimerBlocks) {
                DisplayManager.clearMatrixPart(0, 7, 31, 1);
            }
            break;
    }
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
