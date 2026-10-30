#include "BGDisplayFaceBatteryUptime.h"

#include <esp_timer.h>

#include "globals.h"

namespace {
constexpr int BATTERY_INDICATOR_X = 11;
constexpr int BATTERY_INDICATOR_Y = 0;
constexpr int BATTERY_DOT_COUNT = 10;
}  // namespace

uint16_t BGDisplayFaceBatteryUptime::getBatteryColor() const {
    if (BATTERY_PERCENT < 10) {
        return getBandColor(BG_LEVEL::URGENT_LOW);
    }
    if (BATTERY_PERCENT < 30) {
        return getBandColor(BG_LEVEL::WARNING_LOW);
    }
    return getBandColor(BG_LEVEL::NORMAL);
}

void BGDisplayFaceBatteryUptime::showReadings(
    const std::list<GlucoseReading>& /*readings*/, bool /*dataIsOld*/) const {
    showSystemInfo();
}

void BGDisplayFaceBatteryUptime::showNoData() const { showSystemInfo(); }

// The uptime readout has minute resolution, so re-render on the per-minute tick.
RenderDecision BGDisplayFaceBatteryUptime::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason == RenderReason::TIME_TICK) {
        return RenderDecision::FULL;
    }
    return BGDisplayFace::getRenderDecision(ctx);
}

void BGDisplayFaceBatteryUptime::showSystemInfo() const {
    DisplayManager.clearMatrix();
    // The previous face may have left LARGE selected, which is too wide for this layout.
    DisplayManager.setFont(FONT_TYPE::MEDIUM);

    String batteryText = String(BATTERY_PERCENT) + "%";
    String uptimeText = formatUptime();

    showBatteryIndicator();

    DisplayManager.setTextColor(getBatteryColor());
    DisplayManager.printText(0, 7, batteryText.c_str(), TEXT_ALIGNMENT::LEFT, 2);

    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.printText(31, 7, uptimeText.c_str(), TEXT_ALIGNMENT::RIGHT, 2);
}

void BGDisplayFaceBatteryUptime::showBatteryIndicator() const {
    int filledDots = BATTERY_PERCENT == 100 ? BATTERY_DOT_COUNT : BATTERY_PERCENT / 10;

    for (int i = 0; i < BATTERY_DOT_COUNT; i++) {
        auto color = i < filledDots ? getBatteryColor() : COLOR_GRAY;
        DisplayManager.drawPixel(BATTERY_INDICATOR_X + i, BATTERY_INDICATOR_Y, color);
    }
}

String BGDisplayFaceBatteryUptime::formatUptime() const {
    // esp_timer is 64-bit, unlike millis(), which wraps after about 49.7 days.
    uint64_t uptimeMinutes = esp_timer_get_time() / 1000000ULL / 60;

    if (uptimeMinutes < 100) {
        return String(uptimeMinutes) + "M";
    }

    uint64_t uptimeHours = uptimeMinutes / 60;
    if (uptimeHours < 100) {
        return String(uptimeHours) + "H";
    }

    uint64_t uptimeDays = uptimeHours / 24;
    return String(uptimeDays) + "D";
}
