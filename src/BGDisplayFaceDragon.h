#ifndef BGDISPLAYFACEDRAGON_H
#define BGDISPLAYFACEDRAGON_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"
#include "enums.h"

// A little dragon breathing fire at the glucose value in the glucose colors while the reading is fresh.
// When data is old the fire is out and the dragon is drawn faded in the old-data color.
class BGDisplayFaceDragon : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    unsigned long getAnimationStepMillis() const override;
    void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const override;

private:
    void drawDragon(bool dataIsOld) const;
    void drawFlame(int sgv, unsigned long frame) const;
};

#endif  // BGDISPLAYFACEDRAGON_H
