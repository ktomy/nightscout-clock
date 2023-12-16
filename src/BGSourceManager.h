#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>
#include <list>

#include "enums.h"
#include "DisplayManager.h"

struct GlucoseReading
{
public:
    int sgv;
    BG_TREND trend;
    unsigned long long epoch;

    int getSecondsAgo()
    {
        return time(NULL) - epoch;
    }

    String toString() const
    {
        return String(sgv) + "," + String(trend) + "," + String(epoch);
    }
};

class BGSourceManager_
{
public:
    static BGSourceManager_ &getInstance();

    void setup();
    void tick();
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();

private:
    BGSourceManager_();
    ~BGSourceManager_();

    BGSourceManager_(const BGSourceManager_ &) = delete;
    BGSourceManager_ &operator=(const BGSourceManager_ &) = delete;
};

extern BGSourceManager_ &bgSourceManager; // Declare extern variable

#endif // BGSOURCEMANAGER_H