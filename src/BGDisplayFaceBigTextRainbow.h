#ifndef BGDISPLAYFACEBIGTEXTRAINBOW_H
#define BGDISPLAYFACEBIGTEXTRAINBOW_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

// Big glucose reading in a moving rainbow; the trend arrow shows the glucose range color.
class BGDisplayFaceBigTextRainbow : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void showAnimatedReading(const GlucoseReading& reading) const;
};

#endif  // BGDISPLAYFACEBIGTEXTRAINBOW_H
