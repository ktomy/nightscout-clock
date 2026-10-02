#ifndef BGDISPLAYFACERAINBOWCLOCK_H
#define BGDISPLAYFACERAINBOWCLOCK_H

#include "BGDisplayFace.h"

// Time display with per-character rainbow colors, like Rainbow big text.
class BGDisplayFaceRainbowClock : public BGDisplayFace {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
    bool ticksEverySecond() const override;

private:
    void showTime() const;
};

#endif  // BGDISPLAYFACERAINBOWCLOCK_H
