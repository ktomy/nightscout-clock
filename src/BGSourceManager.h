#ifndef BGSOURCEMANAGER_H
#define BGSOURCEMANAGER_H

#include <Arduino.h>
#include <list>

#include "DisplayManager.h"
#include "enums.h"
#include "BGSource.h"
#include "BGSourceNightscout.h"

class BGSourceManager_ {
  public:
    static BGSourceManager_ &getInstance();

    void setup(BG_SOURCE bgSource);
    void tick();
    bool hasNewData(unsigned long long epochToCompare);
    std::list<GlucoseReading> getGlucoseData();

  private:
    BGSourceManager_();
    ~BGSourceManager_();

    BGSourceManager_(const BGSourceManager_ &) = delete;
    BGSourceManager_ &operator=(const BGSourceManager_ &) = delete;
    BGSource *bgSource;
};

extern BGSourceManager_ &bgSourceManager; // Declare extern variable

#endif // BGSOURCEMANAGER_H