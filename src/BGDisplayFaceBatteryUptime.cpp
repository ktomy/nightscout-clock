#include "BGDisplayFaceBatteryUptime.h"

#include "globals.h"

namespace {
constexpr int BATTERY_INDICATOR_X = 11;
constexpr int BATTERY_INDICATOR_Y = 0;
constexpr int BATTERY_DOT_COUNT = 10;

uint16_t getBatteryColor() {
    if (BATTERY_PERCENT < 10) {
        return BG_COLOR_URGENT;
    }
    if (BATTERY_PERCENT < 30) {
        return BG_COLOR_WARNING;
    }
    return BG_COLOR_NORMAL;
}
}  // namespace

void BGDisplayFaceBatteryUptime::showReadings(
    const std::list<GlucoseReading>& /*readings*/, bool /*dataIsOld*/) const {
    showSystemInfo();
}

void BGDisplayFaceBatteryUptime::showNoData() const { showSystemInfo(); }

bool BGDisplayFaceBatteryUptime::needsFrequentRefresh() const { return true; }

unsigned long BGDisplayFaceBatteryUptime::getFrequentRefreshIntervalMs() const { return 10000; }

void BGDisplayFaceBatteryUptime::showSystemInfo() const {
    DisplayManager.clearMatrix(false);

    String batteryText = String(BATTERY_PERCENT) + "%";
    String uptimeText = formatUptime();

    showBatteryIndicator();

    DisplayManager.setTextColor(getBatteryColor());
    DisplayManager.printText(0, 7, batteryText.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);

    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.printText(31, 7, uptimeText.c_str(), TEXT_ALIGNMENT::RIGHT, 2, false);

    DisplayManager.update();
}

void BGDisplayFaceBatteryUptime::showBatteryIndicator() const {
    int filledDots = BATTERY_PERCENT == 100 ? BATTERY_DOT_COUNT : BATTERY_PERCENT / 10;

    for (int i = 0; i < BATTERY_DOT_COUNT; i++) {
        auto color = i < filledDots ? getBatteryColor() : COLOR_GRAY;
        DisplayManager.drawPixel(BATTERY_INDICATOR_X + i, BATTERY_INDICATOR_Y, color);
    }
}

String BGDisplayFaceBatteryUptime::formatUptime() const {
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long uptimeMinutes = uptimeSeconds / 60;

    if (uptimeMinutes < 100) {
        return String(uptimeMinutes) + "M";
    }

    unsigned long uptimeHours = uptimeMinutes / 60;
    if (uptimeHours < 100) {
        return String(uptimeHours) + "H";
    }

    unsigned long uptimeDays = uptimeHours / 24;
    return String(uptimeDays) + "D";
}
