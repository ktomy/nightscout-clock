#ifndef BGDISPLAYFACECLOCK_H
#define BGDISPLAYFACECLOCK_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"

class BGDisplayFaceClock : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
    void renderPartial(const RenderContext& ctx) const override;

private:
    RenderDecision getAgeTickRenderDecision() const override;
    void showClock() const;
    void clearClockRegion(bool hasTimerBlocks) const;
};

#endif
