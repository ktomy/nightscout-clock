///TODO: To be used for e.g. new data sources


#ifndef BGDisplayManager_h
#define BGDisplayManager_h

#include "NightscoutManager.h"

#include <Arduino.h>
#include <list>

class BGDisplayManager_
{
private:
    std::list<GlucoseReading> displayedReadings;
    void showGlucoseGraph(std::list<GlucoseReading> readings);
    void showReading(GlucoseReading reading);

public:
    static BGDisplayManager_ &getInstance();
    void setup();
    void tick();
    void showData(std::list<GlucoseReading> glucoseReadings);
    unsigned long long getLastDisplayedGlucoseEpoch();
    

};

extern BGDisplayManager_ &BGDisplayManager;
 
#endif