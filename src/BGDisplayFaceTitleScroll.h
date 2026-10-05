#ifndef BGDISPLAYFACETITLESCROLL_H
#define BGDISPLAYFACETITLESCROLL_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"

// Scrolling Nightscout site title followed by the current reading, trend
// arrow, and delta.
class BGDisplayFaceTitleScroll : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void showTitleTrain(const std::list<GlucoseReading>& readings, bool dataIsOld) const;
    void maybeFetchTitle() const;
};

#endif  // BGDISPLAYFACETITLESCROLL_H
