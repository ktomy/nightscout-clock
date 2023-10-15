#ifndef BGDISPLAYFACEGRAPH_H
#define BGDISPLAYFACEGRAPH_H

#include "BGDisplayFace.h"
#include "NightscoutManager.h"

class BGDisplayFaceGraph : public BGDisplayFace
{
public:
    void showReadings(const std::list<GlucoseReading> &readings) const override;
    void markDataAsOld() const override;

private:
    void showReading(GlucoseReading reading) const;
    void showGraph(const std::list<GlucoseReading> &readings) const;
};

#endif