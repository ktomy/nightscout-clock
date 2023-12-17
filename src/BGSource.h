#ifndef BGSOURCE_H
#define BGSOURCE_H

#include <Arduino.h>
#include <list>

#include "enums.h"
#include "globals.h"
#include "DisplayManager.h"

struct GlucoseReading {
  public:
    int sgv;
    BG_TREND trend;
    unsigned long long epoch;

    int getSecondsAgo() { return time(NULL) - epoch; }

    String toString() const { return String(sgv) + "," + ::toString(trend) + "," + String(epoch); }
};

class BGSource {
  public:
    virtual void setup() = 0;
    virtual void tick() = 0;
    virtual bool hasNewData(unsigned long long epochToCompare) const = 0;
    virtual std::list<GlucoseReading> getGlucoseData() const = 0;
};

#endif // BGSOURCE_H
