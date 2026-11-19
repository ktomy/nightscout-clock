#ifndef BGSOURCENIGHTSCOUT_H
#define BGSOURCENIGHTSCOUT_H

#include <LCBUrl.h>

#include "BGSource.h"

class BGSourceNightscout : public BGSource {
public:
private:
    void updateReadings(String baseUrl, String apiKey, bool simplifiedApi, GlucoseReadings& readings);
    std::list<GlucoseReading> retrieveReadings(
        String baseUrl, String apiKey, bool simplifiedApi, unsigned long long lastReadingEpoch,
        unsigned long long readingToEpoch, int numberOfvalues);
    void updateReadings(GlucoseReadings& readings) override;
    LCBUrl prepareUrl(
        String baseUrl, unsigned long long readingSinceEpoch, unsigned long long readingToEpoch,
        int numberOfvalues, bool simplifiedApi);
    int initiateCall(LCBUrl url, bool ssl, String apiKey);
};

#endif  // BGSOURCENIGHTSCOUT_H
