#ifndef BGDISPLAYFACEGRAPHBASE_H
#define BGDISPLAYFACEGRAPHBASE_H

#include "BGDisplayFace.h"
#include "NightscoutManager.h"

class BGDisplayFaceGraphBase : virtual public BGDisplayFace {
  public:
    virtual void showReadings(const std::list<GlucoseReading> &readings) const = 0;
    virtual void markDataAsOld() const = 0;

  protected:
    void showGraphBase(uint8_t x_position, uint8_t length, uint16_t forMinutes, const std::list<GlucoseReading> &readings) const;
};

#endif // BGDISPLAYFACEGRAPHBASE_H
