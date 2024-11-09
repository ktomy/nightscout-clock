#include "BGDisplayManager.h"
#include "DisplayManager.h"
#include "BGSourceManager.h"
#include "BGSource.h"
#include "SettingsManager.h"
#include "ServerManager.h"
#include "globals.h"

#include <list>

// The getter for the instantiated singleton instance
BGDisplayManager_ &BGDisplayManager_::getInstance() {
    static BGDisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGDisplayManager_ &bgDisplayManager = bgDisplayManager.getInstance();

void BGDisplayManager_::setup() {

    glucoseIntervals = GlucoseIntervals();
    /// TODO: Add urgent values to settings

    glucoseIntervals.addInterval(1, SettingsManager.settings.bg_low_urgent_limit, BG_LEVEL::URGENT_LOW);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_low_urgent_limit + 1, SettingsManager.settings.bg_low_warn_limit - 1,
                                 BG_LEVEL::WARNING_LOW);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_low_warn_limit, SettingsManager.settings.bg_high_warn_limit,
                                 BG_LEVEL::NORMAL);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_high_warn_limit, SettingsManager.settings.bg_high_urgent_limit - 1,
                                 BG_LEVEL::WARNING_HIGH);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_high_urgent_limit, 401, BG_LEVEL::URGENT_HIGH);

    faces.push_back(new BGDisplayFaceSimple());
    facesNames[0] = "Simple";
    faces.push_back(new BGDisplayFaceGraph());
    facesNames[1] = "Full graph";
    faces.push_back(new BGDisplayFaceGraphAndBG());
    facesNames[2] = "Graph and BG";
    faces.push_back(new BGDisplayFaceBigText());
    facesNames[3] = "Big text";
    faces.push_back(new BGDisplayFaceValueAndDiff());
    facesNames[4] = "Value and diff";
    faces.push_back(new BGDisplayFaceClock());
    facesNames[5] = "Clock and value";

    currentFaceIndex = SettingsManager.settings.default_clockface;
    if (currentFaceIndex >= faces.size()) {
        currentFaceIndex = 0;
    }

    currentFace = (faces[currentFaceIndex]);
}

std::map<int, String> BGDisplayManager_::getFaces() { return facesNames; }

int BGDisplayManager_::getCurrentFaceId() { return currentFaceIndex; }

GlucoseIntervals BGDisplayManager_::getGlucoseIntervals() { return glucoseIntervals; }

void BGDisplayManager_::setFace(int id) {
    if (id < faces.size()) {
        currentFaceIndex = id;
        currentFace = (faces[currentFaceIndex]);
        DisplayManager.clearMatrix();
        lastRefreshEpoch = 0;
        tick();
    }
}

void BGDisplayManager_::tick() {
    maybeRrefreshScreen();
}

void BGDisplayManager_::maybeRrefreshScreen(bool force) {

    auto currentEpoch = ServerManager.getUtcEpoch();
    tm timeInfo = ServerManager.getTimezonedTime();

    auto lastReading = bgDisplayManager.getLastDisplayedGlucoseReading();

    if (bgSourceManager.hasNewData(lastReading == NULL ? 0 : lastReading->epoch)) {
        DEBUG_PRINTLN("We have new data");
        bgDisplayManager.showData(bgSourceManager.getInstance().getGlucoseData());
        lastRefreshEpoch = currentEpoch;
    } else {
        // We refresh the display every minue trying to match the exact :00 second
        if ( force || timeInfo.tm_sec == 0 && currentEpoch > lastRefreshEpoch || currentEpoch - lastRefreshEpoch > 60) {
            lastRefreshEpoch = currentEpoch;
            if (displayedReadings.size() > 0) {
                bool dataIsOld = displayedReadings.back().getSecondsAgo() > 60 * BG_DATA_OLD_OFFSET_MINUTES;
                currentFace->showReadings(displayedReadings, dataIsOld);
            } else {
                currentFace->showNoData();
            }
        }
    }
}

void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings) {

    if (glucoseReadings.size() == 0) {
        currentFace->showNoData();
        return;
    }

    DisplayManager.clearMatrix();
    currentFace->showReadings(glucoseReadings);

    displayedReadings = glucoseReadings;
}

GlucoseReading *BGDisplayManager_::getLastDisplayedGlucoseReading() {
    if (displayedReadings.size() > 0) {
        return &displayedReadings.back();
    } else {
        return NULL;
    }
}
