#include "BGDisplayManager.h"
#include "globals.h"
#include "NightscoutManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"

#include <list>

// The getter for the instantiated singleton instance
BGDisplayManager_ &BGDisplayManager_::getInstance()
{
    static BGDisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGDisplayManager_ &BGDisplayManager = BGDisplayManager.getInstance();

void BGDisplayManager_::setup() {
    faces.push_back(new BGDisplayFaceSimple());

    currentFace = (faces[0]);
    currentFaceIndex = 0;

}

void BGDisplayManager_::tick() {
    ///TODO: Move back the glucose graph
    ///TODO: Check if the last reading is too old and make it gray

}


void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings) {

    if (glucoseReadings.size() == 0) {
        return;
    }

    currentFace->showReadings(glucoseReadings);

    displayedReadings = glucoseReadings;
}

unsigned long long BGDisplayManager_::getLastDisplayedGlucoseEpoch() {
    if (displayedReadings.size() > 0) {
        return displayedReadings.back().epoch;
    } else {
        return 0;
    }
}
