///TODO: To be used for e.g. new data sources


#ifndef BGDisplayManager_h
#define BGDisplayManager_h

#include "NightscoutManager.h"
#include "BGDisplayFace.h"
#include "BGDisplayFaceSimple.h"

#include <Arduino.h>
#include <list>
#include <vector>

class BGDisplayManager_
{
private:
    std::list<GlucoseReading> displayedReadings;
    std::vector<BGDisplayFace*> faces;
    BGDisplayFace *currentFace;
    int currentFaceIndex;

public:
    static BGDisplayManager_ &getInstance();
    void setup();
    void tick();
    void showData(std::list<GlucoseReading> glucoseReadings);
    unsigned long long getLastDisplayedGlucoseEpoch();
    
};

extern BGDisplayManager_ &BGDisplayManager;
 
#endif