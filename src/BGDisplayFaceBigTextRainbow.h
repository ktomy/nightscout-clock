#ifndef BGDISPLAYFACEBIGTEXTRAINBOW_H
#define BGDISPLAYFACEBIGTEXTRAINBOW_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

// Big glucose reading with an animated rainbow gradient blended over the
// glucose-range color. Blinks while the data is stale.
class BGDisplayFaceBigTextRainbow : public BGDisplayFaceTextBase {
public:
    void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;
    RenderDecision getRenderDecision(const RenderContext& ctx) const override;
    bool ticksEverySecond() const override;

private:
    void showAnimatedReading(const GlucoseReading& reading, bool dataIsOld) const;
};

#endif  // BGDISPLAYFACEBIGTEXTRAINBOW_H
