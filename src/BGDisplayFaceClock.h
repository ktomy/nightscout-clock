#ifndef BGDISPLAYFACECLOCK_H
#define BGDISPLAYFACECLOCK_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceClock : public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld = false) const override;
    void showNoData() const override;

  private:
    void showClock() const;
};

#endif
