#ifndef BGSOURCENIGHTSCOUT_H
#define BGSOURCENIGHTSCOUT_H

#include "BGSource.h"
#include <LCBUrl.h>

class BGSourceNightscout : public BGSource {
  public:
  private:
    std::list<GlucoseReading> updateReadings(String baseUrl, String apiKey, std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(String baseUrl, String apiKey, unsigned long long lastReadingEpoch,
                                               unsigned long long readingToEpoch, int numberOfvalues);
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    LCBUrl prepareUrl(String baseUrl, unsigned long long readingSinceEpoch, unsigned long long readingToEpoch,
                      int numberOfvalues);
    int initiateCall(LCBUrl url, bool ssl, String apiKey);
};

#endif // BGSOURCENIGHTSCOUT_H
