#ifndef BGDISPLAYFACEBIGTEXT_H
#define BGDISPLAYFACEBIGTEXT_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceBigText : public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld = false) const override;
};

#endif
