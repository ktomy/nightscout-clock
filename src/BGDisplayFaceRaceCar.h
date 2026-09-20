#ifndef BGDISPLAYFACERACECAR_H
#define BGDISPLAYFACERACECAR_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"
#include "enums.h"

// A race car speeds toward the value: speed lines in the glucose color stream past, the road slides back
// and the wheels spin. When data is old the whole race stops, drawn faded in the old-data color.
class BGDisplayFaceRaceCar : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void drawRace(int sgv, bool dataIsOld, unsigned long frame) const;
};

#endif  // BGDISPLAYFACERACECAR_H
