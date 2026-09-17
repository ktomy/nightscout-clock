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

// A darker shade of a color. From a fifth of the brightness range up it is the fraction asked for; below
// that the shade is capped so it still lights.
uint16_t shade(uint16_t color, float fraction) {
    const int rich = MIN_BRIGHTNESS + (MAX_BRIGHTNESS - MIN_BRIGHTNESS) / 8;
    if (DisplayManager.getBrightness() < rich) {
        return dim(color);
    }
    const uint16_t r = static_cast<uint16_t>(((color >> 11) & 0x1F) * fraction);
    const uint16_t g = static_cast<uint16_t>(((color >> 5) & 0x3F) * fraction);
    const uint16_t b = static_cast<uint16_t>((color & 0x1F) * fraction);
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

// In the outer quarter of the in-range, low or high band the colour of the band beyond it mixes in; `color` is that
// colour. The middle half of a band, and the urgent bands, show one colour.
bool BGDisplayFaceUnicorn::maneNeighbour(int sgv, BG_LEVEL level, uint16_t& color) const {
    const auto& s = SettingsManager.settings;
    int low, high;
    BG_LEVEL below, above;
    switch (level) {
        case BG_LEVEL::WARNING_LOW:
            low = s.bg_low_urgent_limit, high = s.bg_low_warn_limit;
            below = BG_LEVEL::URGENT_LOW, above = BG_LEVEL::NORMAL;
            break;
        case BG_LEVEL::WARNING_HIGH:
            low = s.bg_high_warn_limit, high = s.bg_high_urgent_limit;
            below = BG_LEVEL::NORMAL, above = BG_LEVEL::URGENT_HIGH;
            break;
        case BG_LEVEL::NORMAL:
            low = s.bg_low_warn_limit, high = s.bg_high_warn_limit;
            below = BG_LEVEL::WARNING_LOW, above = BG_LEVEL::WARNING_HIGH;
            break;
        default:
            return false;
    }
    const int width = high - low;
    if (width <= 0) {
        return false;
    }
    const int quarters = 4 * (sgv - low);
    if (quarters < width) {
        color = getLevelColor(below);
    } else if (quarters > 3 * width) {
        color = getLevelColor(above);
    } else {
        return false;
    }
    return true;
}

void BGDisplayFaceUnicorn::drawMane(int sgv, bool dataIsOld, bool moving, unsigned long frame) const {
    const BG_LEVEL level = bgDisplayManager.getGlucoseIntervals().getBGLevel(sgv);
    const bool inRange = level == BG_LEVEL::NORMAL || level == BG_LEVEL::INVALID;
    const bool urgent = level == BG_LEVEL::URGENT_LOW || level == BG_LEVEL::URGENT_HIGH;
    const MANE_FLOW flow = SettingsManager.settings.face_unicorn.flow;
    const bool wisps = moving && !dataIsOld;
    uint16_t neighbour = 0;
    const bool mixes = moving && !dataIsOld && maneNeighbour(sgv, level, neighbour);

    // Moving tips drift a row up or down into empty cells, so those cells are cleared before the mane is drawn.
    if (wisps) {
        for (int row = 0; row < SPRITE_HEIGHT; row++) {
            for (int col = 0; col < SPRITE_WIDTH; col++) {
                if (UNICORN_ART[row][col] < 'a') {
                    continue;
                }
                for (int near = std::max(0, row - 1); near <= std::min(SPRITE_HEIGHT - 1, row + 1); near++) {
                    if (UNICORN_ART[near][col] == '.') {
                        DisplayManager.drawPixel(col, near, 0);
                    }
                }
            }
        }
    }

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
                    // Every third band holds the neighbouring band's colour when the reading is near it.
                    const uint16_t held = mixes && index % 3 == 0 ? neighbour
                                          : inRange              ? RAINBOW[std::min(4, band)]
                                                                 : getMotionColor(level, 1, index, 0);
                    // A shadow trails the light, so it reads as a comet rather than a blink.
                    color = dash < 2 ? lighten(held) : dash < 6 ? shade(held, 0.62f) : held;
                }
            } else if (moving) {
                // Back: a new color every five columns slides toward the tips, a column a step. The step
                // count wraps after whole color loops, so the motion never jumps.
                const unsigned long step =
                    flow == MANE_FLOW::BACK ? (SPRITE_WIDTH - 1 - col + frame % 30) / 5 : frame;
                color = mixes && (index + step) % 3 == 0 ? neighbour
                        : inRange                         ? RAINBOW[(index + step) % MANE_BANDS]
                                                          : getMotionColor(level, 1, index, step);
            } else if (inRange) {
                color = RAINBOW[std::min(4, band)];
            } else {
                // One color would merge the bands into a block, so odd bands are darker.
                color = band % 2 == 1 ? shade(getLevelColor(level), 0.62f) : getLevelColor(level);
            }
            // A moving tip drifts a row up or down, into an empty cell only, so the mane looks wispy.
            int drawRow = row;
            if (wisps && cell >= 'a') {
                const int phase = static_cast<int>((frame + 2 * col + 3 * row) % 12);
                const int drift = phase < 3 ? -1 : phase < 6 ? 1 : 0;
                const int near = row + drift;
                if (near >= 0 && near < SPRITE_HEIGHT && UNICORN_ART[near][col] == '.') {
                    drawRow = near;
                    DisplayManager.drawPixel(col, row, 0);
                }
            }
            DisplayManager.drawPixel(col, drawRow, cell >= 'a' ? shade(color, 0.5f) : color);
        }
    }
}
