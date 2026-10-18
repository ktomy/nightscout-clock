#ifndef BGDISPLAYFACERAINBOWMOOD_H
#define BGDISPLAYFACERAINBOWMOOD_H

#include "BGDisplayFaceTextBase.h"

// Mood face with a rainbow reading; the expression follows the glucose
// level and trend.
class BGDisplayFaceRainbowMood : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void showAnimatedReading(const GlucoseReading& reading, bool dataIsOld) const;
};

#endif  // BGDISPLAYFACERAINBOWMOOD_H
