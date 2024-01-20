#ifndef BGDISPLAYFACEBIGTEXT_H
#define BGDISPLAYFACEBIGTEXT_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceBigText : public BGDisplayFaceTextBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings) const override;
    void markDataAsOld() const override;

  private:
};

#endif
