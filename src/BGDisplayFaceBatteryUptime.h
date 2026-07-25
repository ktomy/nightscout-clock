#ifndef BGDISPLAYFACEBATTERYUPTIME_H
#define BGDISPLAYFACEBATTERYUPTIME_H

#include "BGDisplayFace.h"

class BGDisplayFaceBatteryUptime : public BGDisplayFace {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;

private:
    void showSystemInfo() const;
    void showBatteryIndicator() const;
    String formatUptime() const;
};

#endif
