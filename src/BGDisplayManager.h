#ifndef BGDisplayManager_h
#define BGDisplayManager_h

#include <Arduino.h>

#include <list>
#include <vector>

#include "BGDisplayFace.h"
#include "BGDisplayFaceBigText.h"
#include "BGDisplayFaceBigTextDark.h"
#include "BGDisplayFaceClock.h"
#include "BGDisplayFaceDragon.h"
#include "BGDisplayFaceGraph.h"
#include "BGDisplayFaceGraphAndBG.h"
#include "BGDisplayFaceRaceCar.h"
#include "BGDisplayFaceSimple.h"
#include "BGDisplayFaceSimpleDark.h"
#include "BGDisplayFaceTimeOnly.h"
#include "BGDisplayFaceUnicorn.h"
#include "BGDisplayFaceValueAndDiff.h"
#include "BGSource.h"
#include "SettingsSchedule.h"

struct GlucoseInterval {
    int low_boundary;
    int high_boundary;
    BG_LEVEL intarval_type;
};

struct GlucoseIntervals {
    std::vector<GlucoseInterval> intervals;  // Use a vector to store the intervals

    // Method to add a GlucoseInterval to the array
    void addInterval(int low, int high, BG_LEVEL type) { intervals.push_back({low, high, type}); }

    BG_LEVEL getBGLevel(int value) const {
        for (const GlucoseInterval& interval : intervals) {
            if (value >= interval.low_boundary && value <= interval.high_boundary) {
                return interval.intarval_type;
            }
        }
        return BG_LEVEL::INVALID;  // Default to INVALID if not found in any interval
    }

    String toString() const {
        String ss = "ColorIntervals:\n";
        for (const GlucoseInterval& interval : intervals) {
            ss += "  Low: " + String(interval.low_boundary) +
                  ", High: " + String(interval.high_boundary) + ", Level: ";
            switch (interval.intarval_type) {
                case BG_LEVEL::URGENT_HIGH:
                    ss += "URGENT_HIGH";
                    break;
                case BG_LEVEL::WARNING_HIGH:
                    ss += "WARNING_HIGH";
                    break;
                case BG_LEVEL::NORMAL:
                    ss += "NORMAL";
                    break;
                case BG_LEVEL::WARNING_LOW:
                    ss += "WARNING_LOW";
                    break;
                case BG_LEVEL::URGENT_LOW:
                    ss += "URGENT_LOW";
                    break;
                case BG_LEVEL::INVALID:
                    ss += "INVALID";
                    break;
            }
        }
        return ss;
    }
};

struct RegisteredClockFace {
    String id;
    String name;
    BGDisplayFace* instance;
};

class BGDisplayManager_ {
private:
    std::list<GlucoseReading> displayedReadings;
    std::vector<RegisteredClockFace> faces;
    BGDisplayFace* currentFace;
    int currentFaceIndex;
    GlucoseIntervals glucoseIntervals;
    bool lastRenderedDataWasOld = false;
    bool faceCycleActive = false;
    bool faceCycleTimerStarted = false;
    unsigned long lastFaceCycleMillis = 0;
    std::vector<int> activeFaces;
    unsigned long lastAnimationFrame = 0;
    std::vector<FaceScheduleEntry> faceSchedule;  // sorted by start time
    bool faceScheduleActive = false;
    int appliedScheduleEntry = -1;
    int lastScheduleMinuteOfDay = -1;
    bool brightnessOverlayWasActive = false;

    bool drawAnimationFrame(bool dataIsOld, bool redraw);
    int findFaceIndex(const String& id) const;
    void setFaceByIndex(int index);
    void configureActiveFaces();
    void updateFaceCycle();
    void configureFaceSchedule();
    void updateFaceSchedule();
    void applyScheduleEntry(const FaceScheduleEntry& entry);
    void resetFaceCycleTimer();
    void runRenderCycle(RenderReason reason, const tm& timeInfo);
    void commitRenderedState(bool dataIsOld);

public:
    static BGDisplayManager_& getInstance();
    void setup();
    void tick();
    void maybeRrefreshScreen(bool force = false);
    void showData(std::list<GlucoseReading> glucoseReadings);
    GlucoseReading* getLastDisplayedGlucoseReading();
    const GlucoseIntervals& getGlucoseIntervals() const;

    const std::vector<RegisteredClockFace>& getFaces() const;
    String getCurrentFaceId() const;
    bool suppressesNewAlarms() const;

    void setFace(const String& id);
    void showNextFace();
    void showPreviousFace();

private:
    unsigned long long lastRefreshEpoch = 0;
};

extern BGDisplayManager_& bgDisplayManager;

#endif
