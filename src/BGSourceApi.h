#ifndef BGSOURCEAPI_H
#define BGSOURCEAPI_H

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include "BGSource.h"

class BGSourceApi : public BGSource {
public:
    bool hasNewData(unsigned long long epoch) override;

private:
    void setup() override;
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    void HandleEntriesPost(AsyncWebServerRequest* request, JsonVariant& json);
    AsyncCallbackWebHandler* createDeleteEntriesHandler();
    bool hasNewDataFlag = false;
};

#endif  // BGSOURCEAPI_H