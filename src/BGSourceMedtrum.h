

#ifndef BGSOURCEMEDTRUM_H
#define BGSOURCEMEDTRUM_H

#include <ArduinoJson.h>
#include <DisplayManager.h>
#include <ServerManager.h>
#include <SettingsManager.h>

#include "BGSource.h"

// Implements Medtrum Easy Follow glucose data source

class BGSourceMedtrum : public BGSource {
public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    void setup() override;

private:
    String sessionCookie;
    bool login();
    bool isLoggedIn() const { return sessionCookie != ""; }
    GlucoseReading getCurrentGlucose(String& username, bool& ok);
    std::list<GlucoseReading> getHistory(const String& username, time_t from, time_t to, bool& ok);
};

#endif  // BGSOURCEMEDTRUM_H
