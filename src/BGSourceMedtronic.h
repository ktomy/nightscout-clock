#ifndef BGSOURCEMEDTRONIC_H
#define BGSOURCEMEDTRONIC_H

#include "BGSource.h"

class BGSourceMedtronic : public BGSource {
public:
    void updateReadings(GlucoseReadings& readings) override;

private:
};

#endif  // BGSOURCEMEDTRONIC_H
