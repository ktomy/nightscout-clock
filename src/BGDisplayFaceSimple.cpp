#include "BGDisplayFaceSimple.h"
#include "BGDisplayManager.h"
#include "globals.h"

#include <ctime> // Include for time tracking

void BGDisplayFaceSimple::showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld) const {
    static time_t lastUpdateTime = 0; // Track the last update time
    int blockCount = 0; // Number of blocks to display

    DisplayManager.clearMatrix();

    showReading(readings.back(), 0, 6, TEXT_ALIGNMENT::CENTER, FONT_TYPE::MEDIUM, dataIsOld);

    // Show arrow in the right part of the screen
    showTrendArrow(readings.back(), 32 - 5, 1);

    // Calculate elapsed minutes since the last update
    time_t currentTime = time(nullptr);
    if (lastUpdateTime != 0) {
        int elapsedMinutes = (currentTime - lastUpdateTime) / 60;
        blockCount = (elapsedMinutes > 5) ? 5 : elapsedMinutes; // Cap blocks at 5
    }

    // Draw blocks at the bottom of the display
    int blockWidth = 5;  // Width of each block
    int blockHeight = 2; // Height of each block
    int spacing = 2;     // Spacing between blocks
    int startX = 2;      // Starting X position
    int startY = 31;     // Y position at the bottom of the screen

    for (int i = 0; i < blockCount; i++) {
        int x = startX + i * (blockWidth + spacing);
        DisplayManager.drawRect(x, startY, blockWidth, blockHeight, true); // Draw filled rectangle
    }

    // Update the last update time after displaying
    lastUpdateTime = currentTime;
}
