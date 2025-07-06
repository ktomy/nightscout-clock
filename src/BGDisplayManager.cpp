#include "BGDisplayManager.h"

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

void BGDisplayManager_::tick() { maybeRrefreshScreen(); }

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
        if (force || timeInfo.tm_sec == 0 && currentEpoch > lastRefreshEpoch ||
            currentEpoch - lastRefreshEpoch > 60) {
            lastRefreshEpoch = currentEpoch;
            if (displayedReadings.size() > 0) {
                bool dataIsOld = displayedReadings.back().getSecondsAgo() >
                                 60 * SettingsManager.settings.bg_data_too_old_threshold_minutes;
                DisplayManager.clearMatrix();
                currentFace->showReadings(displayedReadings, dataIsOld);
                DisplayManager.update();
            } else {
                DisplayManager.clearMatrix();
                currentFace->showNoData();
                DisplayManager.update();
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

// We draw the horizontal blocks equal to the number of minutes since last reading
// maximum numer of lines is 5
// Depending on the face we draw the lines in different places having different sizes
// The idea is to fit that maximum of 5 lines in the available space
// We can draw lines in 3 colors:
// - dark green if reading is less than 6 minutes old
// - dark orange if reading is between 6 and old_data_threshold_minutes threshold
// - gray if reading is older than old_data_threshold_minutes threshold
// @param lastReading - the last reading to draw the lines for
// @param width - the width of the available space in pixels
// @param yPosition - the y position of the lines
// @param xPosition - the x position of the lines
void BGDisplayManager_::drawTimerBlocks(
    GlucoseReading lastReading, int width, int xPosition, int yPosition) {
    const int MAX_BLOXCS = 5;  // maximum number of blocks to draw

    int blocksCount = lastReading.getSecondsAgo() / 60;
    if (blocksCount > MAX_BLOXCS) {
        blocksCount = MAX_BLOXCS;  // we draw maximum 5 lines
    }
    if (blocksCount <= 0) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("No blocks to draw, not drawing timer blocks");
#endif
        return;
    }

    // minimal block size is 1 pixel, size between blocks is 1 pixel, so we get width, subtract spaces
    // between lines and divide by the maximum number of lines
    int blockSize = blockSize = (width - 4) / MAX_BLOXCS;
    if (blockSize < 1) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Block size is less than 1, not drawing timer blocks");
#endif
        return;
    }

    // Now let's alter xPosition to center the blocks in the available space
    xPosition += (width - (blockSize * MAX_BLOXCS + (MAX_BLOXCS - 1))) / 2;

    uint16_t color = COLOR_DARK_GREEN;
    if (lastReading.getSecondsAgo() >= 60 * SettingsManager.settings.bg_data_too_old_threshold_minutes) {
    	if (SettingsManager.settings.dimmer_mode_enable) {
            color = COLOR_DIMMER_TIMER_BLOCK_OLD;  // dimmer profile old data
    	}
    	else {
            	color = COLOR_GRAY;  // old data
    	}
    } else if (lastReading.getSecondsAgo() >= (MAX_BLOXCS + 1) * 60) {
    	if (SettingsManager.settings.dimmer_mode_enable) {
            color = COLOR_DIMMER_TIMER_BLOCK_WARNING;  // dimmer profile warning data
    	}
    	else {
        color = COLOR_DARK_ORANGE;  // warning data
    	}
    } else if (SettingsManager.settings.dimmer_mode_enable) {
        color = COLOR_DIMMER_TIMER_BLOCK;  // dimmer profile
    }
#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF(
        "Drawing %d blocks of size %d at position (%d, %d) with color %04X", blocksCount, blockSize,
        xPosition, yPosition, color);
#endif

    for (int i = 0; i < blocksCount; i++) {
        int x = xPosition + i * (blockSize + 1);  // +1 for the space between blocks
        for (int j = 0; j < blockSize; j++) {
            DisplayManager.drawPixel(x + j, yPosition, color, false);
        }
    }
}

GlucoseReading* BGDisplayManager_::getLastDisplayedGlucoseReading() {
    if (displayedReadings.size() > 0) {
        return &displayedReadings.back();
    } else {
        return NULL;
    }
}
