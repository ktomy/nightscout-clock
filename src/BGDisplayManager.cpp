#include "BGDisplayManager.h"

#include <algorithm>
#include <list>

#include "BGSource.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "ServerManager.h"
#include "SettingsManager.h"
#include "globals.h"

// The getter for the instantiated singleton instance
BGDisplayManager_& BGDisplayManager_::getInstance() {
    static BGDisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGDisplayManager_& bgDisplayManager = bgDisplayManager.getInstance();

void BGDisplayManager_::setup() {
    glucoseIntervals = GlucoseIntervals();
    /// TODO: Add urgent values to settings

    glucoseIntervals.addInterval(1, SettingsManager.settings.bg_low_urgent_limit, BG_LEVEL::URGENT_LOW);
    glucoseIntervals.addInterval(
        SettingsManager.settings.bg_low_urgent_limit + 1, SettingsManager.settings.bg_low_warn_limit - 1,
        BG_LEVEL::WARNING_LOW);
    glucoseIntervals.addInterval(
        SettingsManager.settings.bg_low_warn_limit, SettingsManager.settings.bg_high_warn_limit,
        BG_LEVEL::NORMAL);
    glucoseIntervals.addInterval(
        SettingsManager.settings.bg_high_warn_limit, SettingsManager.settings.bg_high_urgent_limit - 1,
        BG_LEVEL::WARNING_HIGH);
    glucoseIntervals.addInterval(
        SettingsManager.settings.bg_high_urgent_limit, 401, BG_LEVEL::URGENT_HIGH);

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
    faces.push_back(new BGDisplayFaceUnicorn());
    facesNames[6] = "Unicorn";
    faces.push_back(new BGDisplayFaceSimpleDark());
    facesNames[7] = "Simple (dark)";
    faces.push_back(new BGDisplayFaceBigTextDark());
    facesNames[8] = "Big text (dark)";

    if (faces.size() != CLOCK_FACE_COUNT) {
        DEBUG_PRINTF(
            "Face count mismatch: %u registered, CLOCK_FACE_COUNT is %d",
            static_cast<unsigned int>(faces.size()), CLOCK_FACE_COUNT);
    }

    configureActiveFaces();
    configureFaceSchedule();

    if (faceCycleActive) {
        currentFaceIndex = activeFaces.front();
    } else {
        currentFaceIndex = SettingsManager.settings.default_clockface;
    }

    if (currentFaceIndex < 0 || static_cast<size_t>(currentFaceIndex) >= faces.size()) {
        currentFaceIndex = 0;
    }

    currentFace = (faces[currentFaceIndex]);
}

// The active faces are the ones the buttons move between, and the ones cycling runs through.
void BGDisplayManager_::configureActiveFaces() {
    activeFaces.clear();
    faceCycleActive = false;
    faceCycleTimerStarted = false;

    const std::vector<int>& inactiveFaces = SettingsManager.settings.inactive_faces;
    for (int faceId = 0; static_cast<size_t>(faceId) < faces.size(); faceId++) {
        if (std::find(inactiveFaces.begin(), inactiveFaces.end(), faceId) == inactiveFaces.end()) {
            activeFaces.push_back(faceId);
        }
    }

    if (!SettingsManager.settings.face_cycle_enabled) {
        return;
    }

    if (activeFaces.size() < 2) {
        DEBUG_PRINTF(
            "Clock face cycling disabled: at least two active faces are required, found %u\n",
            static_cast<unsigned int>(activeFaces.size()));
        return;
    }

    faceCycleActive = true;
}

std::map<int, String> BGDisplayManager_::getFaces() { return facesNames; }

int BGDisplayManager_::getCurrentFaceId() { return currentFaceIndex; }

GlucoseIntervals BGDisplayManager_::getGlucoseIntervals() { return glucoseIntervals; }

void BGDisplayManager_::setFace(int id) {
    if (id < 0 || static_cast<size_t>(id) >= faces.size()) {
        return;
    }

    currentFaceIndex = id;
    currentFace = (faces[currentFaceIndex]);
    lastRefreshEpoch = 0;
    resetFaceCycleTimer();
    runRenderCycle(RenderReason::FACE_CHANGE, ServerManager.getTimezonedTime());
}

void BGDisplayManager_::showNextFace() {
    if (activeFaces.empty()) {
        return;
    }

    auto current = std::find(activeFaces.begin(), activeFaces.end(), currentFaceIndex);
    if (current == activeFaces.end()) {
        setFace(activeFaces.front());
        return;
    }

    current++;
    setFace(current == activeFaces.end() ? activeFaces.front() : *current);
}

void BGDisplayManager_::showPreviousFace() {
    if (activeFaces.empty()) {
        return;
    }

    auto current = std::find(activeFaces.begin(), activeFaces.end(), currentFaceIndex);
    if (current == activeFaces.end() || current == activeFaces.begin()) {
        setFace(activeFaces.back());
    } else {
        setFace(*--current);
    }
}

void BGDisplayManager_::resetFaceCycleTimer() {
    lastFaceCycleMillis = millis();
    faceCycleTimerStarted = true;
}

void BGDisplayManager_::updateFaceCycle() {
    if (!faceCycleActive) {
        return;
    }

    if (MATRIX_OFF) {
        faceCycleTimerStarted = false;
        return;
    }

    unsigned long currentMillis = millis();
    if (!faceCycleTimerStarted) {
        lastFaceCycleMillis = currentMillis;
        faceCycleTimerStarted = true;
        return;
    }

    unsigned long intervalMillis =
        static_cast<unsigned long>(SettingsManager.settings.face_cycle_interval_seconds) * 1000UL;
    if (currentMillis - lastFaceCycleMillis >= intervalMillis) {
        showNextFace();
    }
}

void BGDisplayManager_::tick() {
    updateFaceSchedule();
    updateFaceCycle();
    maybeRrefreshScreen();
}

// Cycling and the schedule both own the face, so cycling wins when both are on.
void BGDisplayManager_::configureFaceSchedule() {
    faceSchedule = SettingsManager.settings.face_schedule;
    std::sort(
        faceSchedule.begin(), faceSchedule.end(),
        [](const FaceScheduleEntry& a, const FaceScheduleEntry& b) {
            return a.startMinutes < b.startMinutes;
        });
    appliedScheduleEntry = -1;
    lastScheduleMinuteOfDay = -1;
    faceScheduleActive =
        SettingsManager.settings.face_schedule_enabled && !faceCycleActive && !faceSchedule.empty();
}

// The row in force is the latest one passed today, else the last row; each row re-applies daily
// at its time, even a single row. No known time means no row applies.
void BGDisplayManager_::updateFaceSchedule() {
    if (!faceScheduleActive) {
        return;
    }

    static unsigned long lastCheckMillis = 0;
    if (millis() - lastCheckMillis < 1000) {
        return;
    }
    lastCheckMillis = millis();

    tm now;
    if (!ServerManager.tryGetTimezonedTime(now)) {
        return;
    }
    const int minuteOfDay = now.tm_hour * 60 + now.tm_min;

    int current = static_cast<int>(faceSchedule.size()) - 1;
    for (size_t i = 0; i < faceSchedule.size(); i++) {
        if (faceSchedule[i].startMinutes <= minuteOfDay) {
            current = static_cast<int>(i);
        }
    }

    const bool reachedRowTime = minuteOfDay == faceSchedule[current].startMinutes &&
                                minuteOfDay != lastScheduleMinuteOfDay;
    lastScheduleMinuteOfDay = minuteOfDay;

    if (current == appliedScheduleEntry && !reachedRowTime) {
        return;
    }
    appliedScheduleEntry = current;
    applyScheduleEntry(faceSchedule[current]);
}

// Applied the way the Web UI or the buttons would: the brightness settings change in memory,
// so the automatic modes carry on from there, and the face is switched.
void BGDisplayManager_::applyScheduleEntry(const FaceScheduleEntry& entry) {
    DEBUG_PRINTF("Schedule: face %d, brightness %d\n", entry.face, entry.brightness);
    if (entry.brightness >= 100) {
        SettingsManager.settings.brightness_mode = static_cast<BRIGHTNES_MODE>(entry.brightness);
    } else {
        SettingsManager.settings.brightness_mode = BRIGHTNES_MODE::MANUAL;
        SettingsManager.settings.brightness_level = entry.brightness - 1;
    }
    DisplayManager.applySettings();
    setFace(entry.face);
}

void BGDisplayManager_::commitRenderedState(bool dataIsOld) {
    lastRenderedDataWasOld = dataIsOld;
    lastRefreshEpoch = ServerManager.getUtcEpoch();
}

void BGDisplayManager_::runRenderCycle(RenderReason reason, const tm& timeInfo) {
    bool dataIsOld = displayedReadings.size() > 0 &&
                     displayedReadings.back().getSecondsAgo() >
                         60 * SettingsManager.settings.bg_data_too_old_threshold_minutes;
    RenderContext ctx{reason, timeInfo, dataIsOld, lastRenderedDataWasOld, displayedReadings};

    switch (currentFace->getRenderDecision(ctx)) {
        case RenderDecision::NONE:
            return;
        case RenderDecision::PARTIAL:
            currentFace->renderPartial(ctx);
            DisplayManager.update();
            commitRenderedState(dataIsOld);
            return;
        case RenderDecision::FULL:
            DisplayManager.clearMatrix();
            if (displayedReadings.size() > 0) {
                currentFace->showReadings(displayedReadings, dataIsOld);
            } else {
                currentFace->showNoData();
            }
            DisplayManager.update();
            commitRenderedState(dataIsOld);
            return;
    }
}

void BGDisplayManager_::maybeRrefreshScreen(bool force) {
    auto currentEpoch = ServerManager.getUtcEpoch();
    tm timeInfo = ServerManager.getTimezonedTime();

    auto lastReading = bgDisplayManager.getLastDisplayedGlucoseReading();

    if (bgSourceManager.hasNewData(lastReading == NULL ? 0 : lastReading->epoch)) {
        DEBUG_PRINTLN("We have new data");
        bgDisplayManager.showData(bgSourceManager.getInstance().getGlucoseData());
    } else {
        // We refresh the display every minue trying to match the exact :00 second
        if (force) {
            runRenderCycle(RenderReason::FORCED, timeInfo);
        } else if (
            timeInfo.tm_sec == 0 && currentEpoch > lastRefreshEpoch ||
            currentEpoch - lastRefreshEpoch > 60) {
            runRenderCycle(RenderReason::TIME_TICK, timeInfo);
        }
    }
}

void BGDisplayManager_::showData(std::list<GlucoseReading> glucoseReadings) {
    if (glucoseReadings.size() == 0) {
        displayedReadings.clear();
        runRenderCycle(RenderReason::NEW_DATA, ServerManager.getTimezonedTime());
        return;
    }

    displayedReadings = glucoseReadings;
    runRenderCycle(RenderReason::NEW_DATA, ServerManager.getTimezonedTime());
}

GlucoseReading* BGDisplayManager_::getLastDisplayedGlucoseReading() {
    if (displayedReadings.size() > 0) {
        return &displayedReadings.back();
    } else {
        return NULL;
    }
}
