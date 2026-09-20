#ifndef BGDISPLAYFACESIMPLEDARK_H
#define BGDISPLAYFACESIMPLEDARK_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

// Simple for a dark room: the number in one configured color, the glucose range color on the
// trend arrow, and no age bars.
class BGDisplayFaceSimpleDark : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
};

#endif
