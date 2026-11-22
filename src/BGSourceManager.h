#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>

#include "BGSource.h"
#include "BGSourceApi.h"
#include "BGSourceDexcom.h"
#include "BGSourceLibreLinkUp.h"
#include "BGSourceMedtronic.h"
#include "BGSourceMedtrum.h"
#include "BGSourceNightscout.h"
#include "DisplayManager.h"
#include "enums.h"

class BGSourceManager_ {
public:
    static BGSourceManager_& getInstance();

    void setup(BG_SOURCE bgSource);
    void tick();
    // Stops the data source for good before a firmware update, whichever source it is (BGSource::end).
    // The clock restarts after the update, so nothing starts it again.
    void end();
    bool isStopped() const { return bgSource != nullptr && bgSource->isStopped(); }
    bool hasNewData(unsigned long long epochToCompare);
    const GlucoseReadings& getGlucoseData();
    // A copy of the newest reading, for the web server task: the readings change in the main loop.
    GlucoseReading getLatestReading() const { return latestReading; }
    BG_SOURCE getCurrentSourceType() const { return currentSourceType; }
    BGSource* getSource() const { return bgSource; }
    String getSourceStatus() const { return (bgSource) ? bgSource->getStatus() : "unknown"; }

private:
    BGSourceManager_();
    ~BGSourceManager_();

    BGSourceManager_(const BGSourceManager_&) = delete;
    BGSourceManager_& operator=(const BGSourceManager_&) = delete;
    BGSource* bgSource = nullptr;
    BG_SOURCE currentSourceType;
    unsigned long long lastPollEpoch = 0;
    GlucoseReading latestReading{0, BG_TREND::NONE, 0};
};

extern BGSourceManager_& bgSourceManager;  // Declare extern variable

#endif  // BGSOURCEMANAGER_H
