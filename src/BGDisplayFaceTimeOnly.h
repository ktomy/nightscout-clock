#ifndef BGDISPLAYFACETIMEONLY_H
#define BGDISPLAYFACETIMEONLY_H

#include "BGDisplayFace.h"

// Always show only the time and suppress new glucose alarms.
class BGDisplayFaceTimeOnly : public BGDisplayFace {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
    bool ticksEverySecond() const override;
    bool suppressesNewAlarms() const override;

private:
    void showTime() const;
};

#endif  // BGDISPLAYFACETIMEONLY_H
