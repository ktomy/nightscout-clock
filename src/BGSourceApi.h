#ifndef BGSOURCEAPI_H
#define BGSOURCEAPI_H

#include "BGSource.h"
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

class BGSourceApi : public BGSource {
  public:
  private:
    void setup() override;
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    void HandleEntriesPost(AsyncWebServerRequest *request, JsonVariant &json);
};

#endif // BGSOURCEAPI_H