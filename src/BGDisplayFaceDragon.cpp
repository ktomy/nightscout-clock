#include "BGDisplayFaceDragon.h"

#include <algorithm>
#include <cmath>

#include "BGDisplayManager.h"
#include "globals.h"

namespace {

const int SPRITE_WIDTH = 16;
const int SPRITE_HEIGHT = 8;
const int FLAME_END = SPRITE_WIDTH - 1;

// S body, D belly, W wing, H horn, E eye, T tooth, 0-2 flame from core to edge, t flame tip.
// The flame runs from the mouth at column 8 to column 15, so column 16 stays dark before the reading.
const char* const DRAGON_ART[SPRITE_HEIGHT] = {
    ".WW...H.........",  //
    "WWWW.SSH......t.",  //
    ".WWWSSESS...22.t",  //
    "..SSSSSST.22112.",  //
    "S.SSSSS.00011122",  //
    "SSDDSSST.11122.t",  //
    ".S.S.S.....22.t.",  //
    "....S...........",  //
};

// Different hues, since shades of one hue look alike on the LEDs; none of them is a glucose color.
const uint16_t BODY = 0x023F;     // blue
const uint16_t BELLY = 0xA79F;    // pale cyan
const uint16_t WING = 0x073F;     // cyan
const uint16_t WHITE = 0xFFFF;    // horn, eye and tooth
const uint16_t EYE_OLD = 0x18C3;  // near black

uint16_t dragonColor(char cell) {
    switch (cell) {
        case 'S':
            return BODY;
        case 'D':
            return BELLY;
        case 'W':
            return WING;
        default:
            return WHITE;
    }
}

}  // namespace

unsigned long BGDisplayFaceDragon::getAnimationStepMillis() const {
    return getStepMillis(SettingsManager.settings.face_dragon.speed);
}

void BGDisplayFaceDragon::drawDragon(bool dataIsOld) const {
    const uint16_t staleColor = getDataOldColor();
    for (int row = 0; row < SPRITE_HEIGHT; row++) {
        for (int col = 0; col < SPRITE_WIDTH; col++) {
            const char cell = DRAGON_ART[row][col];
            if (cell == '.' || cell == 't' || (cell >= '0' && cell <= '2')) {
                continue;
            }
            if (!dataIsOld) {
                DisplayManager.drawPixel(col, row, dragonColor(cell));
            } else if ((row + col) % 2 == 0) {
                // Every other pixel, so a stale dragon looks faded in any old-data color.
                DisplayManager.drawPixel(col, row, cell == 'E' ? EYE_OLD : staleColor);
            }
        }
    }
}

void BGDisplayFaceDragon::drawFlame(int sgv, unsigned long frame) const {
    const BG_LEVEL level = bgDisplayManager.getGlucoseIntervals().getBGLevel(sgv);
    const int quarter = getWarningQuarter(sgv, level);
    const bool inRange = level == BG_LEVEL::NORMAL || level == BG_LEVEL::INVALID;
    // Fire rolls out one layer every two steps and flickers over twelve, so the loop is twelve steps.
    const int roll = (frame / 2) % 3;
    const float phase = (frame % 12) * (2 * 3.14159265f / 12);

    for (int row = 0; row < SPRITE_HEIGHT; row++) {
        for (int col = 0; col < SPRITE_WIDTH; col++) {
            const char cell = DRAGON_ART[row][col];
            if (cell != 't' && (cell < '0' || cell > '2')) {
                continue;
            }
            // Layers 0-2 are the flame from the core out and 3 is a tip; some pixels flicker a layer
            // hotter or cooler, and a pixel cooler than a tip goes out.
            int layer = cell == 't' ? 3 : ((cell - '0' - roll) % 3 + 3) % 3;
            const float flicker = std::sin(phase - col * 0.9f + row * 1.3f);
            if (flicker > 0.7f) {
                layer = std::max(0, layer - 1);
            } else if (flicker < -0.7f) {
                layer++;
            }
            if (layer > 3) {
                continue;
            }

            int drawRow = row;
            if (layer == 3) {
                const float bob = std::sin(phase - col * 0.8f - row);
                const int shift = bob > 0.5f ? -1 : bob < -0.5f ? 1 : 0;
                drawRow = std::min(SPRITE_HEIGHT - 1, std::max(0, row + shift));
            }
            // In range the fire is the first four in-range colours, magenta at the core out to violet.
            // Out of range the motion colors are counted back from the end of the flame, so they run out
            // of the mouth.
            const uint16_t color =
                inRange ? IN_RANGE_COLORS[layer]
                        : getMotionColor(level, quarter, FLAME_END - col, frame);
            DisplayManager.drawPixel(col, drawRow, color);
        }
    }
}

void BGDisplayFaceDragon::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    DisplayManager.clearMatrixPart(0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
    drawFlame(readings.back().sgv, frame);
    drawDragon(false);
}

void BGDisplayFaceDragon::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    const GlucoseReading& lastReading = readings.back();

    drawDragon(dataIsOld);

    showReading(lastReading, MATRIX_WIDTH - 1, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);

    // Age indicator lives to the right of the sprite.
    drawTimerBlocks(lastReading, 16, 16, 7);
}
