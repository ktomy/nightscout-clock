#ifndef BGDISPLAYFACEVALUEANDDIFF_H
#define BGDISPLAYFACEVALUEANDDIFF_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceValueAndDiff : public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld = false) const override;

  private:
    String getDiff(const std::list<GlucoseReading> &readings) const;
};

#endif
