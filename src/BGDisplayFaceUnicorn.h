#ifndef BGDISPLAYFACEUNICORN_H
#define BGDISPLAYFACEUNICORN_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"
#include "enums.h"

// The unicorn's hair is rainbow for normal readings and takes the range's color otherwise.
// When data is old, the unicorn uses the configured stale color, keeping its eye dark.
// The glucose value is shown as a smaller readout beside it.
class BGDisplayFaceUnicorn : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
};

#endif  // BGDISPLAYFACEUNICORN_H
