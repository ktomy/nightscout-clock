#ifndef BGDISPLAYFACETIMEONLY_H
#define BGDISPLAYFACETIMEONLY_H

#include "BGDisplayFaceClock.h"

// An ordinary clock that hides glucose. It shows the "Clock and value" layout instead while a
// fresh reading is urgent or a glucose alarm is active, so the reading can be seen when it matters.
class BGDisplayFaceTimeOnly : public BGDisplayFaceClock {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
    bool ticksEverySecond() const override;

private:
    bool isUrgent(const GlucoseReading& reading) const;
    void showTime() const;
};

#endif  // BGDISPLAYFACETIMEONLY_H
