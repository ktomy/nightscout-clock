#ifndef BGSOURCENIGHTSCOUT_H
#define BGSOURCENIGHTSCOUT_H

#include "BGSource.h"

class BGSourceNightscout : public BGSource {
  public:
  private:
    std::list<GlucoseReading> updateReadings(String baseUrl, String apiKey, std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(String baseUrl, String apiKey, unsigned long long lastReadingEpoch,
                                               unsigned long long readingToEpoch, int numberOfvalues);
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
};

#endif // BGSOURCENIGHTSCOUT_H
