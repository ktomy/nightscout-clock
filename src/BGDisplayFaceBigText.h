#ifndef BGDISPLAYFACEBIGTEXT_H
#define BGDISPLAYFACEBIGTEXT_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceBigText : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;

private:
    // True from the configured early-stale minute until the data-is-old threshold.
    bool isEarlyStale(const GlucoseReading& reading, bool dataIsOld) const;
};

#endif
