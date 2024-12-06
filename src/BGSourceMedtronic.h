#ifndef BGSOURCEMEDTRONIC_H
#define BGSOURCEMEDTRONIC_H

#include "BGSource.h"

class BGSourceMedtronic : public BGSource {
  public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;

  private:
};

#endif // BGSOURCEMEDTRONIC_H