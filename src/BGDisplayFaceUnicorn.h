#ifndef BGDISPLAYFACEUNICORN_H
#define BGDISPLAYFACEUNICORN_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"
#include "enums.h"

// The unicorn's mane is color bands that break into dim tips: magenta to gold for normal readings and
// warning/urgent colors otherwise. A moving mane rolls those colors while the reading is fresh.
// When data is old, the unicorn uses the configured stale color, keeping its eye dark.
// The glucose value is shown as a smaller readout beside it.
class BGDisplayFaceUnicorn : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void drawMane(int sgv, bool dataIsOld, bool moving, unsigned long frame) const;
};

#endif  // BGDISPLAYFACEUNICORN_H
