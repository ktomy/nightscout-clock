#ifndef BGDisplayManager_h
#define BGDisplayManager_h

#include "BGDisplayFace.h"
#include "BGDisplayFaceGraph.h"
#include "BGDisplayFaceGraphAndBG.h"
#include "BGDisplayFaceSimple.h"
#include "BGDisplayFaceBigText.h"
#include "BGDisplayFaceValueAndDiff.h"
#include "BGDisplayFaceClock.h"
#include "BGSource.h"

#include <Arduino.h>
#include <list>
#include <map>
#include <vector>

struct GlucoseInterval {
    int low_boundary;
    int high_boundary;
    BG_LEVEL intarval_type;
};

struct GlucoseIntervals {
    std::vector<GlucoseInterval> intervals; // Use a vector to store the intervals

    // Method to add a GlucoseInterval to the array
    void addInterval(int low, int high, BG_LEVEL type) { intervals.push_back({low, high, type}); }

    BG_LEVEL getBGLevel(int value) {
        for (const GlucoseInterval &interval : intervals) {
            if (value >= interval.low_boundary && value <= interval.high_boundary) {
                return interval.intarval_type;
            }
        }
        return BG_LEVEL::INVALID; // Default to INVALID if not found in any interval
    }

    String toString() const {
        String ss = "ColorIntervals:\n";
        for (const GlucoseInterval &interval : intervals) {
            ss += "  Low: " + String(interval.low_boundary) + ", High: " + String(interval.high_boundary) + ", Level: ";
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

class BGDisplayManager_ {
  private:
    std::list<GlucoseReading> displayedReadings;
    std::vector<BGDisplayFace *> faces;
    BGDisplayFace *currentFace;
    int currentFaceIndex;
    GlucoseIntervals glucoseIntervals;
    std::map<int, String> facesNames;

  public:
    static BGDisplayManager_ &getInstance();
    void setup();
    void tick();
    void maybeRrefreshScreen(bool force = false);
    void showData(std::list<GlucoseReading> glucoseReadings);
    GlucoseReading *getLastDisplayedGlucoseReading();
    GlucoseIntervals getGlucoseIntervals();

    std::map<int, String> getFaces();
    int getCurrentFaceId();

    void setFace(int id);

    static void drawTimerBlocks(int elapsedMinutes, int maxBlocks, bool dataIsOld);

  private:
    unsigned long long lastRefreshEpoch;
};

extern BGDisplayManager_ &bgDisplayManager;

#endif
