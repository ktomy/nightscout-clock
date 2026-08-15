#include "BGDisplayFaceWithAge.h"

#include "SettingsManager.h"
#include "globals.h"

RenderDecision BGDisplayFaceWithAge::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason != RenderReason::TIME_TICK || ctx.readings.empty()) {
        return BGDisplayFace::getRenderDecision(ctx);
    }

    if (ctx.dataIsOld != ctx.wasDataOld) {
        return RenderDecision::FULL;
    }

    return getAgeTickRenderDecision();
}

RenderDecision BGDisplayFaceWithAge::getAgeTickRenderDecision() const { return RenderDecision::FULL; }

// Draw horizontal blocks equal to the number of minutes since the last reading, up to five blocks.
void BGDisplayFaceWithAge::drawTimerBlocks(
    GlucoseReading lastReading, int width, int xPosition, int yPosition) const {
    const int MAX_BLOCKS = 5;

    int blocksCount = lastReading.getSecondsAgo() / 60;
    if (blocksCount > MAX_BLOCKS) {
        blocksCount = MAX_BLOCKS;
    }
    if (blocksCount <= 0) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("No blocks to draw, not drawing timer blocks");
#endif
        return;
    }

    int blockSize = (width - 4) / MAX_BLOCKS;
    if (blockSize < 1) {
#ifdef DEBUG_DISPLAY
        DEBUG_PRINTLN("Block size is less than 1, not drawing timer blocks");
#endif
        return;
    }

    xPosition += (width - (blockSize * MAX_BLOCKS + (MAX_BLOCKS - 1))) / 2;

    uint16_t color = COLOR_GREEN;
    if (lastReading.getSecondsAgo() >= 60 * SettingsManager.settings.bg_data_too_old_threshold_minutes) {
        color = COLOR_GRAY;
    } else if (lastReading.getSecondsAgo() >= (MAX_BLOCKS + 1) * 60) {
        color = COLOR_YELLOW;
    }
#ifdef DEBUG_DISPLAY
    DEBUG_PRINTF(
        "Drawing %d blocks of size %d at position (%d, %d) with color %04X", blocksCount, blockSize,
        xPosition, yPosition, color);
#endif

    for (int i = 0; i < blocksCount; i++) {
        int x = xPosition + i * (blockSize + 1);
        for (int j = 0; j < blockSize; j++) {
            DisplayManager.drawPixel(x + j, yPosition, color, false);
        }
    }
}
