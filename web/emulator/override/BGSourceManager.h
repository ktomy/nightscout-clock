// Replaces the firmware's BGSourceManager.h: the readings come from the page instead of a data source.
#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>

#include "BGSource.h"
#include "enums.h"

class BGSourceManager_ {
public:
    static BGSourceManager_& getInstance();
    bool hasNewData(unsigned long long epochToCompare);
    const GlucoseReadings& getGlucoseData();

    GlucoseReadings readings;
};

extern BGSourceManager_& bgSourceManager;

#endif
