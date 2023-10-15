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

void BGDisplayManager_::setup()
{

    glucoseIntervals = GlucoseIntervals();
    /// TODO: Add urgent values to settings

    glucoseIntervals.addInterval(1, 55, URGENT_LOW);
    glucoseIntervals.addInterval(56, SettingsManager.settings.bgLow - 1, WARNING_LOW);
    glucoseIntervals.addInterval(SettingsManager.settings.bgLow, SettingsManager.settings.bgHigh, NORMAL);
    glucoseIntervals.addInterval(SettingsManager.settings.bgHigh, 260, WARNING_HIGH);
    glucoseIntervals.addInterval(260, 401, URGENT_HIGH);

    faces.push_back(new BGDisplayFaceSimple());
    faces.push_back(new BGDisplayFaceGraph());

    currentFaceIndex = 0;
    currentFace = (faces[currentFaceIndex]);
}

GlucoseIntervals BGDisplayManager_::getGlucoseIntervals()
{
    return glucoseIntervals;
}

void BGDisplayManager_::tick()
{
    /// TODO: Move back the glucose graph
    /// TODO: Check if the last reading is too old and make it gray
}

void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings)
{

    if (glucoseReadings.size() == 0)
    {
        return;
    }

    currentFace->showReadings(glucoseReadings);

    displayedReadings = glucoseReadings;
}

unsigned long long BGDisplayManager_::getLastDisplayedGlucoseEpoch()
{
    if (displayedReadings.size() > 0)
    {
        return displayedReadings.back().epoch;
    }
    else
    {
        return 0;
    }
}
