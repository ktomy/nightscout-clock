#ifndef BGDISPLAYFACEBATTERYUPTIME_H
#define BGDISPLAYFACEBATTERYUPTIME_H

#include "BGDisplayFace.h"

// Battery indicator, battery percentage, and uptime. Disabled by default;
// enable it in the web UI when needed.
class BGDisplayFaceBatteryUptime : public BGDisplayFace {
public:
    void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;

private:
    uint16_t getBatteryColor() const;
    void showSystemInfo() const;
    void showBatteryIndicator() const;
    String formatUptime() const;
};

#endif  // BGDISPLAYFACEBATTERYUPTIME_H
