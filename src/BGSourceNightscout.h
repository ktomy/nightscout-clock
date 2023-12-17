#include "BGSource.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <list>

class BGSourceNightscout : public BGSource {
  public:
    void setup() override;
    void tick() override;
    bool hasNewData(unsigned long long epochToCompare) const override;
    std::list<GlucoseReading> getGlucoseData() const override;

  private:
    HTTPClient *client;
    WiFiClientSecure *wifiSecureClient;
    WiFiClient *wifiClient;
    // unsigned long lastReadingEpoch;
    std::list<GlucoseReading> glucoseReadings;
    bool firstConnectionSuccess;
    std::list<GlucoseReading> updateReadings(String baseUrl, String apiKey, std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(String baseUrl, String apiKey, unsigned long long lastReadingEpoch,
                                               unsigned long long readingToEpoch, int numberOfvalues);
    std::list<GlucoseReading> deleteOldReadings(std::list<GlucoseReading> readings, unsigned long long epochToCompare);
};