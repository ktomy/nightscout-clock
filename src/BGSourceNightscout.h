#ifndef BGSOURCENIGHTSCOUT_H
#define BGSOURCENIGHTSCOUT_H

#include <LCBUrl.h>

#include "BGSource.h"

class BGSourceNightscout : public BGSource {
public:
private:
    std::list<GlucoseReading> updateReadings(
        String baseUrl, String apiKey, bool simplifiedApi, std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(
        String baseUrl, String apiKey, bool simplifiedApi, unsigned long long lastReadingEpoch,
        unsigned long long readingToEpoch, int numberOfvalues);
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    LCBUrl prepareUrl(
        String baseUrl, unsigned long long readingSinceEpoch, unsigned long long readingToEpoch,
        int numberOfvalues, bool simplifiedApi);
    int initiateCall(LCBUrl url, bool ssl, String apiKey);
};

#endif  // BGSOURCENIGHTSCOUT_H
