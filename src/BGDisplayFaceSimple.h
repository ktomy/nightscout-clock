#ifndef BGDISPLAYFACESIMPLE_H
#define BGDISPLAYFACESIMPLE_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceSimple : public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings) const override;
    void markDataAsOld() const override;

  private:
};

#endif