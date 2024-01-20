#include "BGDisplayManager.h"
#include "DisplayManager.h"
#include "BGSource.h"
#include "SettingsManager.h"
#include "globals.h"

#include <list>

// The getter for the instantiated singleton instance
BGDisplayManager_ &BGDisplayManager_::getInstance() {
    static BGDisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGDisplayManager_ &BGDisplayManager = BGDisplayManager.getInstance();

void BGDisplayManager_::setup() {

    glucoseIntervals = GlucoseIntervals();
    /// TODO: Add urgent values to settings

    glucoseIntervals.addInterval(1, 55, BG_LEVEL::URGENT_LOW);
    glucoseIntervals.addInterval(56, SettingsManager.settings.bg_low_warn_limit - 1, BG_LEVEL::WARNING_LOW);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_low_warn_limit, SettingsManager.settings.bg_high_warn_limit,
                                 BG_LEVEL::NORMAL);
    glucoseIntervals.addInterval(SettingsManager.settings.bg_high_warn_limit, 260, BG_LEVEL::WARNING_HIGH);
    glucoseIntervals.addInterval(260, 401, BG_LEVEL::URGENT_HIGH);

    faces.push_back(new BGDisplayFaceSimple());
    facesNames[0] = "Simple";
    faces.push_back(new BGDisplayFaceGraph());
    facesNames[1] = "Full graph";
    faces.push_back(new BGDisplayFaceGraphAndBG());
    facesNames[2] = "Graph and BG";
    faces.push_back(new BGDisplayFaceBigText());
    facesNames[3] = "Big text";

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
        if (displayedReadings.size() > 0) {
            currentFace->showReadings(displayedReadings);
        } else {
            DisplayManager.setTextColor(COLOR_GRAY);
            DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
        }
    }
}

void BGDisplayManager_::tick() {
    /// TODO: Check if the last reading is too old and make it gray
}

void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings) {

    if (glucoseReadings.size() == 0) {
        DisplayManager.setTextColor(COLOR_GRAY);
        DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
        return;
    }

    DisplayManager.clearMatrix();
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
