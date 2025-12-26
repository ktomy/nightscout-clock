#ifndef BGSOURCEMEDTRUM_H
#define BGSOURCEMEDTRUM_H

#include "BGSource.h"

class BGSourceMedtrum : public BGSource {
public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    void setup() override;

private:
    // Add Medtrum-specific members here (e.g., email, password) if needed
};

#endif  // BGSOURCEMEDTRUM_H
