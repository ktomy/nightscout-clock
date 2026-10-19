#ifndef BGDISPLAYFACEWITHAGE_H
#define BGDISPLAYFACEWITHAGE_H

#include "BGDisplayFace.h"

class BGDisplayFaceWithAge : virtual public BGDisplayFace {
public:
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;

protected:
    void drawTimerBlocks(GlucoseReading lastReading, int width, int xPosition, int yPosition) const;
    // Single-pixel age dots on the bottom row under a trend arrow at the right edge, for faces whose
    // number fills the rows above.
    void drawAgeDotsUnderArrow(const GlucoseReading& lastReading) const;
    virtual RenderDecision getAgeTickRenderDecision() const;
};

#endif  // BGDISPLAYFACEWITHAGE_H
