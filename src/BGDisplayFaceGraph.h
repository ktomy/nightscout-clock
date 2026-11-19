#ifndef BGDISPLAYFACEGRAPH_H
#define BGDISPLAYFACEGRAPH_H

#include "BGDisplayFaceGraphBase.h"

class BGDisplayFaceGraph : public BGDisplayFaceGraphBase {
public:
    void showReadings(const GlucoseReadings& readings, bool dataIsOld = false) const override;
};

#endif
