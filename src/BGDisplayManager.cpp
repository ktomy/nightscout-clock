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

    // Stable IDs identify faces; registration order only controls navigation.
    faces = {
        {"simple", "Simple", new BGDisplayFaceSimple()},
        {"graph", "Full graph", new BGDisplayFaceGraph()},
        {"graph_and_bg", "Graph and BG", new BGDisplayFaceGraphAndBG()},
        {"big_text", "Big text", new BGDisplayFaceBigText()},
        {"value_and_diff", "Value and diff", new BGDisplayFaceValueAndDiff()},
        {"clock", "Clock and value", new BGDisplayFaceClock()},
        {"unicorn", "Unicorn", new BGDisplayFaceUnicorn()},
        {"time_only", "Time only", new BGDisplayFaceTimeOnly()},
        {"simple_dark", "Simple (dark)", new BGDisplayFaceSimpleDark()},
        {"race_car", "Race car", new BGDisplayFaceRaceCar()},
        {"dragon", "Dragon", new BGDisplayFaceDragon()},
        {"big_text_dark", "Big text (dark)", new BGDisplayFaceBigTextDark()},
        {"diagnostics", "Diagnostics", new BGDisplayFaceDiagnostics()},
        {"battery_uptime", "Battery and uptime", new BGDisplayFaceBatteryUptime()},
        {"big_text_rainbow", "Rainbow big text", new BGDisplayFaceBigTextRainbow()},
        {"smiley", "Smiley", new BGDisplayFaceSmiley()},
    };

    configureActiveFaces();
    configureFaceSchedule();

    if (faceCycleActive) {
        currentFaceIndex = activeFaces.front();
    } else {
        currentFaceIndex = findFaceIndex(SettingsManager.settings.default_clockface);
    }

    if (currentFaceIndex < 0 || static_cast<size_t>(currentFaceIndex) >= faces.size()) {
        currentFaceIndex = 0;
    }

    currentFace = faces[currentFaceIndex].instance;
    currentFace->onActivate();
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
}

// The active faces are the ones the buttons move between, and the ones cycling runs through.
void BGDisplayManager_::configureActiveFaces() {
    activeFaces.clear();
    faceCycleActive = false;
    faceCycleTimerStarted = false;

    const std::vector<String>& inactiveFaces = SettingsManager.settings.inactive_faces;
    for (int index = 0; static_cast<size_t>(index) < faces.size(); index++) {
        if (std::find(inactiveFaces.begin(), inactiveFaces.end(), faces[index].id) == inactiveFaces.end()) {
            activeFaces.push_back(index);
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

const std::vector<RegisteredClockFace>& BGDisplayManager_::getFaces() const { return faces; }

String BGDisplayManager_::getCurrentFaceId() const { return faces[currentFaceIndex].id; }

int BGDisplayManager_::findFaceIndex(const String& id) const {
    for (size_t index = 0; index < faces.size(); index++) {
        if (faces[index].id == id) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool BGDisplayManager_::suppressesNewAlarms() const {
    return currentFace->suppressesNewAlarms();
}

const GlucoseIntervals& BGDisplayManager_::getGlucoseIntervals() const { return glucoseIntervals; }

void BGDisplayManager_::setFace(const String& id) { setFaceByIndex(findFaceIndex(id)); }

void BGDisplayManager_::setFaceByIndex(int index) {
    if (index < 0 || static_cast<size_t>(index) >= faces.size()) {
        return;
    }

    currentFaceIndex = index;
    currentFace = faces[currentFaceIndex].instance;
    currentFace->onActivate();
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
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
        setFaceByIndex(activeFaces.front());
        return;
    }

    current++;
    setFaceByIndex(current == activeFaces.end() ? activeFaces.front() : *current);
}

void BGDisplayManager_::showPreviousFace() {
    if (activeFaces.empty()) {
        return;
    }

    auto current = std::find(activeFaces.begin(), activeFaces.end(), currentFaceIndex);
    if (current == activeFaces.end() || current == activeFaces.begin()) {
        setFaceByIndex(activeFaces.back());
    } else {
        setFaceByIndex(*--current);
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
    if (!MATRIX_OFF && currentFace->needsFrequentRefresh() &&
        millis() - lastFrequentRefreshMillis >= currentFace->getFrequentRefreshIntervalMs()) {
        lastFrequentRefreshMillis = millis();
        runRenderCycle(RenderReason::FORCED, ServerManager.getTimezonedTime());
    }
    if (!MATRIX_OFF && drawAnimationFrame(lastRenderedDataWasOld, false)) {
        DisplayManager.update();
    }
}

// Draws the moving part of an animated face for the current step while the reading is fresh.
// Returns false when there is nothing to draw, or `redraw` is false and this step is already shown.
bool BGDisplayManager_::drawAnimationFrame(bool dataIsOld, bool redraw) {
    const unsigned long stepMillis = currentFace->getAnimationStepMillis();
    if (stepMillis == 0 || dataIsOld || displayedReadings.empty()) {
        return false;
    }

    const unsigned long frame = millis() / stepMillis;
    if (frame == lastAnimationFrame && !redraw) {
        return false;
    }
    lastAnimationFrame = frame;
    currentFace->showAnimationFrame(displayedReadings, frame);
    return true;
}

// Cycling and the schedule both own the face, so cycling wins when both are on.
void BGDisplayManager_::configureFaceSchedule() {
    faceSchedule.clear();
    for (const FaceScheduleEntry& entry : SettingsManager.settings.face_schedule) {
        if (findFaceIndex(entry.face) >= 0) {
            faceSchedule.push_back(entry);
        }
    }
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
    DEBUG_PRINTF("Schedule: face %s, brightness %d\n", entry.face.c_str(), entry.brightness);
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
                drawAnimationFrame(dataIsOld, true);
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
        // We refresh the display every minute trying to match the exact :00 second,
        // or every second for faces that ask for it
        if (force) {
            runRenderCycle(RenderReason::FORCED, timeInfo);
        } else if (
            (timeInfo.tm_sec == 0 || currentFace->ticksEverySecond()) &&
                currentEpoch > lastRefreshEpoch ||
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
