#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>

#include <list>

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
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();
    BG_SOURCE getCurrentSourceType() const { return currentSourceType; }
    String getSourceStatus() const { return (bgSource) ? bgSource->getStatus() : "unknown"; }

private:
    BGSourceManager_();
    ~BGSourceManager_();

    BGSourceManager_(const BGSourceManager_&) = delete;
    BGSourceManager_& operator=(const BGSourceManager_&) = delete;
    BGSource* bgSource = nullptr;
    BG_SOURCE currentSourceType;
    unsigned long long lastPollEpoch = 0;
};

extern BGSourceManager_& bgSourceManager;  // Declare extern variable

#endif  // BGSOURCEMANAGER_H