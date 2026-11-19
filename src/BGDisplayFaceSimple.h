#ifndef BGDISPLAYFACESIMPLE_H
#define BGDISPLAYFACESIMPLE_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"
#include "BGSource.h"

class BGDisplayFaceSimple : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const GlucoseReadings& readings, bool dataIsOld = false) const override;
};

#endif
