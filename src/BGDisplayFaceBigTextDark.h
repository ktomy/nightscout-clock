#ifndef BGDISPLAYFACEBIGTEXTDARK_H
#define BGDISPLAYFACEBIGTEXTDARK_H

#include "BGDisplayFaceSimpleDark.h"
#include "BGDisplayFaceWithAge.h"

// Simple (dark) with Big text's placement and font, readable across a dark room, with optional age dots
// under the trend arrow.
class BGDisplayFaceBigTextDark : public BGDisplayFaceSimpleDark, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
};

#endif
