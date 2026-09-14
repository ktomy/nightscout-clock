#ifndef BGDISPLAYFACESIMPLEDARK_H
#define BGDISPLAYFACESIMPLEDARK_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

// A night version of the Simple face: the reading in the per-band colours from the face
// settings (all white by default), the glucose band on the trend arrow, no age blocks.
class BGDisplayFaceSimpleDark : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;

private:
    uint16_t getValueColor(const GlucoseReading& reading) const;
};

#endif
