#ifndef BGSOURCELIBRELINKUP_H
#define BGSOURCELIBRELINKUP_H

#include "BGSource.h"

class BGSourceLibreLinkUp : public BGSource {

  public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;

  private:
};

#endif // BGSOURCELIBRELINKUP_H