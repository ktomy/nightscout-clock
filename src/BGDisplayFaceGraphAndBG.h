// same as BGDisplayFaceGraph.h but with glucose value
#ifndef BGDISPLAYFACEGRAPHANDBG_H
#define BGDISPLAYFACEGRAPHANDBG_H

#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayFaceTextBase.h"

class BGDisplayFaceGraphAndBG : public BGDisplayFaceGraphBase, public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings) const override;
    void markDataAsOld() const override;
    BGDisplayFaceGraphAndBG() {}
};

#endif // BGDISPLAYFACEGRAPHANDBG_H
