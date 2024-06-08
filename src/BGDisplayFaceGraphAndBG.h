// same as BGDisplayFaceGraph.h but with glucose value
#ifndef BGDISPLAYFACEGRAPHANDBG_H
#define BGDISPLAYFACEGRAPHANDBG_H

#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayFaceTextBase.h"

class BGDisplayFaceGraphAndBG : public BGDisplayFaceGraphBase, public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld = false) const override;

  private:
    void showTrendVerticalLine(int x, BG_TREND trend) const;
};

#endif // BGDISPLAYFACEGRAPHANDBG_H
