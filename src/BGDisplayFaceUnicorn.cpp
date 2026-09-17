#include "BGDisplayFaceUnicorn.h"

#include <algorithm>
#include <cstring>

#include "BGDisplayManager.h"
#include "globals.h"

namespace {

const int SPRITE_WIDTH = 17;
const int SPRITE_HEIGHT = 8;
const int MANE_BANDS = 6;

// H horn, W body, E eye, 0-5 mane band from top to bottom, r o y g b m the same bands as broken tips.
const char* const UNICORN_ART[SPRITE_HEIGHT] = {
    "HH...r0000r......",  //
    ".HH.00111000r...r",  //
    "..HH011WW11100rro",  //
    "...WWWWW22221111y",  //
    "WWWWWEWW33322222g",  //
    "WWWWWWWW44443333b",  //
    ".WWWWWWW.554444bb",  //
    "...WWWWWW...555m.",  //
};
const char* const TIPS = "roygbm";

const uint16_t HORN = 0xFF58;  // gold
const uint16_t BODY = 0xF79D;  // cream white
const uint16_t EYE = 0x18C3;   // near black

// In range, top band to bottom: magenta, pink, coral, peach and gold; purple closes the moving loop.
// Each keeps its blue lit at the lowest brightness, where an orange would show as the urgent red.
const uint16_t RAINBOW[MANE_BANDS] = {0xF81F, 0xFA38, 0xFC98, 0xFE38, 0xFF58, 0xB19F};

// The mane band of a cell or of its tip, or -1 for the rest of the unicorn.
int bandOf(char cell) {
    if (cell >= '0' && cell <= '5') {
        return cell - '0';
    }
    const char* tip = cell == '\0' ? nullptr : std::strchr(TIPS, cell);
    return tip != nullptr ? static_cast<int>(tip - TIPS) : -1;
}

// The darkest shade of a color whose lit channels still light at the lowest brightness.
uint16_t dim(uint16_t color) {
    const uint16_t r = std::min<uint16_t>((color >> 11) & 0x1F, 24);
    const uint16_t g = std::min<uint16_t>((color >> 5) & 0x3F, 46);
    const uint16_t b = std::min<uint16_t>(color & 0x1F, 24);
    return (r << 11) | (g << 5) | b;
}

}  // namespace

void BGDisplayFaceUnicorn::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();

    const uint16_t staleColor = getDataOldColor();
    for (int row = 0; row < SPRITE_HEIGHT; row++) {
        for (int col = 0; col < SPRITE_WIDTH; col++) {
            const char cell = UNICORN_ART[row][col];
            if (cell == '.' || bandOf(cell) >= 0) {
                continue;
            }
            // Keep the eye dark so it remains distinct from the stale-colored face.
            const uint16_t color = cell == 'E'   ? EYE
                                   : dataIsOld   ? staleColor
                                   : cell == 'H' ? HORN
                                                 : BODY;
            DisplayManager.drawPixel(col, row, color);
        }
    }
    drawMane(lastReading.sgv, dataIsOld, false, 0);

    // The reading and the age bars end at the right edge, clear of the mane's tips.
    showReading(lastReading, MATRIX_WIDTH + 1, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    drawTimerBlocks(lastReading, 16, 17, 7);
}

unsigned long BGDisplayFaceUnicorn::getAnimationStepMillis() const {
    const auto& unicorn = SettingsManager.settings.face_unicorn;
    return unicorn.mane_moving ? getStepMillis(unicorn.speed) : 0;
}

void BGDisplayFaceUnicorn::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    drawMane(readings.back().sgv, false, true, frame);
}

void BGDisplayFaceUnicorn::drawMane(int sgv, bool dataIsOld, bool moving, unsigned long frame) const {
    const BG_LEVEL level = bgDisplayManager.getGlucoseIntervals().getBGLevel(sgv);
    const int quarter = getWarningQuarter(sgv, level);
    const bool inRange = level == BG_LEVEL::NORMAL || level == BG_LEVEL::INVALID;
    const bool urgent = level == BG_LEVEL::URGENT_LOW || level == BG_LEVEL::URGENT_HIGH;
    const MANE_FLOW flow = SettingsManager.settings.face_unicorn.flow;

    for (int row = 0; row < SPRITE_HEIGHT; row++) {
        for (int col = 0; col < SPRITE_WIDTH; col++) {
            const char cell = UNICORN_ART[row][col];
            const int band = bandOf(cell);
            if (band < 0) {
                continue;
            }
            // Counted from the bottom band, so the colors flow down the mane.
            const int index = MANE_BANDS - 1 - band;
            uint16_t color;
            if (dataIsOld) {
                color = getDataOldColor();
            } else if (moving && flow == MANE_FLOW::RUN) {
                // The bands hold their colors while a light runs along each toward the tips, half a
                // column a step; past an urgent limit the light is a dark stripe.
                const int dash = ((2 * col - static_cast<int>(frame % 16) + 3 * band) % 16 + 16) % 16;
                if (urgent) {
                    color = dash % 8 < 2 ? 0 : getLevelColor(level);
                } else {
                    const uint16_t held =
                        inRange ? RAINBOW[std::min(4, band)] : getMotionColor(level, quarter, index, 0);
                    color = dash < 2 ? lighten(held) : held;
                }
            } else if (moving) {
                // Back: a new color every five columns slides toward the tips, a column a step. The step
                // count wraps after whole color loops, so the motion never jumps.
                const unsigned long step =
                    flow == MANE_FLOW::BACK ? (SPRITE_WIDTH - 1 - col + frame % 30) / 5 : frame;
                color = inRange ? RAINBOW[(index + step) % MANE_BANDS]
                                : getMotionColor(level, quarter, index, step);
            } else if (inRange) {
                color = RAINBOW[std::min(4, band)];
            } else {
                // One color would merge the bands into a block, so odd bands are darker.
                color = band % 2 == 1 ? dim(getLevelColor(level)) : getLevelColor(level);
            }
            DisplayManager.drawPixel(col, row, cell >= 'a' ? dim(color) : color);
        }
    }
}
