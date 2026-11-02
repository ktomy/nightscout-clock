// Replaces the firmware's BGSourceManager.h: the readings come from the page instead of a data source.
#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>

#include <list>

#include "BGSource.h"
#include "enums.h"

class BGSourceManager_ {
public:
    static BGSourceManager_& getInstance();
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();

    std::list<GlucoseReading> readings;
};

extern BGSourceManager_& bgSourceManager;

#endif
