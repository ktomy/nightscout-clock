#ifndef BGDISPLAYFACESIMPLE_H
#define BGDISPLAYFACESIMPLE_H

#include "BGDisplayFace.h"
#include "NightscoutManager.h"

class BGDisplayFaceSimple : public BGDisplayFace {
public:
    void showReadings(const std::list<GlucoseReading>& readings) const override;
    void markDataAsOld() const override;
private:
    void showReading(GlucoseReading reading) const;

};

#endif