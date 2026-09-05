#ifndef BGDISPLAYFACEUNICORN_H
#define BGDISPLAYFACEUNICORN_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"
#include "enums.h"

// Mascot-led face: a unicorn sprite carries the alarm state through its mane color
// (rainbow when normal, flattened to the warning/urgent/stale color otherwise), with
// the glucose value as a smaller readout beside it.
class BGDisplayFaceUnicorn : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;

private:
    const uint16_t* getManePalette(BG_LEVEL level, bool dataIsOld) const;
};

#endif  // BGDISPLAYFACEUNICORN_H
