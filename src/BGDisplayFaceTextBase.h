#ifndef BGDISPLAYFACETEXTBASE_H
#define BGDISPLAYFACETEXTBASE_H

#include "BGDisplayFace.h"
#include "NightscoutManager.h"

class BGDisplayFaceTextBase : public BGDisplayFace {
  public:
    virtual void showReadings(const std::list<GlucoseReading> &readings) const = 0;
    virtual void markDataAsOld() const = 0;

  protected:
    void showTrendArrow(const GlucoseReading reading, int16_t x, int16_t y) const;
    void showReadingBase(const GlucoseReading reading, int16_t x, int16_t y, bool centered = false) const;

};

#endif // BGDISPLAYFACETEXTBASE_H
