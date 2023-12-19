#ifndef BGSOURCEDEXCOM_H
#define BGSOURCEDEXCOM_H

#include "BGSource.h"

class BGSourceDexcom : public BGSource {
  public:
  private:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
};

#endif // BGSOURCEDEXCOM_H
