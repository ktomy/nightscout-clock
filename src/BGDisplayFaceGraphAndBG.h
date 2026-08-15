// same as BGDisplayFaceGraph.h but with glucose value
#ifndef BGDISPLAYFACEGRAPHANDBG_H
#define BGDISPLAYFACEGRAPHANDBG_H

#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"

class BGDisplayFaceGraphAndBG : public BGDisplayFaceGraphBase,
                                public BGDisplayFaceTextBase,
                                public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
};

#endif  // BGDISPLAYFACEGRAPHANDBG_H
