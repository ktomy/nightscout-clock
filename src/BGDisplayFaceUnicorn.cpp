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

// The mane band of a cell or of its tip, or -1 for the rest of the unicorn.
int bandOf(char cell) {
    if (cell >= '0' && cell <= '5') {
        return cell - '0';
    }
    const char* tip = cell == '\0' ? nullptr : std::strchr(TIPS, cell);
    return tip != nullptr ? static_cast<int>(tip - TIPS) : -1;
}

// Every mane pattern repeats after this many steps, so the motion never jumps when the step count wraps.
const unsigned long LOOP_STEPS = 240;

// A cheap integer hash: the same inputs always give the same value, so a random-looking mane is still a
// function of the step number.
uint32_t scramble(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u + b * 0x85EBCA77u + 0x27D4EB2Fu;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return h;
}

// How far a column is behind the nearest light running along a band, in half columns: 0-1 is the light,
// 2-5 its shadow, more the band color. Each band starts its lights at irregular times and each light
// runs at a whole or a half half-column a step, so the bands never move in step.
int runDash(int band, int col, unsigned long frame) {
    const unsigned long SLOT = 12;
    const unsigned long step = frame % LOOP_STEPS;
    int nearest = 16;
    const unsigned long SLOTS = LOOP_STEPS / SLOT;
    for (unsigned long slot = 0; slot < SLOTS; slot++) {
        const uint32_t h = scramble(band + 1, slot);
        // A slot may skip its light, but never two in a row, so no band sits still for long.
        if (h % 8 >= 5 && scramble(band + 1, (slot + SLOTS - 1) % SLOTS) % 8 < 5) {
            continue;
        }
        const unsigned long start = slot * SLOT + (h >> 8) % SLOT;
        const unsigned long elapsed = (step + LOOP_STEPS - start) % LOOP_STEPS;
        const int dash = 2 * col - static_cast<int>(elapsed / (1 + (h >> 16) % 2));
        if (dash >= 0 && dash < nearest) {
            nearest = dash;
        }
    }
    return nearest;
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

// Twinkles: each mane cell lights up at its own random moments, in TWINKLE_CHANCE eighths of its
// TWINKLE_PERIOD-step windows, for TWINKLE_STEPS steps.
const int TWINKLE_PERIOD = 16;
const int TWINKLE_CHANCE = 5;
const int TWINKLE_STEPS = 4;

// How many steps into a twinkle a cell is, or -1 when it is not twinkling.
int twinkleAge(int cell, unsigned long frame) {
    const unsigned long offset = scramble(cell, 99) % TWINKLE_PERIOD;
    const unsigned long step = (frame % LOOP_STEPS + offset) % LOOP_STEPS;
    const uint32_t h = scramble(cell + 7, step / TWINKLE_PERIOD);
    if (static_cast<int>(h % 8) >= TWINKLE_CHANCE) {
        return -1;
    }
    const int age = static_cast<int>(step % TWINKLE_PERIOD) -
                    static_cast<int>((h >> 8) % (TWINKLE_PERIOD - TWINKLE_STEPS));
    return age >= 0 && age < TWINKLE_STEPS ? age : -1;
}

// At the lowest brightness each LED channel is only on or off, so the in-range colors collapse to
// magenta and blue. There the mane is magenta, blue and cyan from the top, two bands each: never white
// (it would merge with the body), and blue and cyan cannot pass for the old-data or early-stale colors,
// which color the whole unicorn.
const uint16_t LOWEST_MANE[3] = {0xF81F, 0x001F, 0x07FF};

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

    // The reading sits two dark columns from the mane's tips; one too wide for that moves out to the
    // right edge. The age bars stay at the edge.
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    const int width = DisplayManager.getTextWidth(getPrintableReading(lastReading.sgv).c_str(), 2);
    const bool fits = MATRIX_WIDTH - 1 - width >= SPRITE_WIDTH + 2;
    showReading(
        lastReading, fits ? MATRIX_WIDTH - 1 : MATRIX_WIDTH + 1, 6, TEXT_ALIGNMENT::RIGHT,
        FONT_TYPE::MEDIUM, dataIsOld);
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

// In the outer quarter of the in-range, low or high band the color of the band beyond it mixes in; `color` is that
// color. The middle half of a band, and the urgent bands, show one color.
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
    // The run flow at the lowest brightness uses its own colors, and its tips stay put so they leave no
    // dark cell.
    const bool lowest = moving && !dataIsOld && flow == MANE_FLOW::RUN &&
                        DisplayManager.getBrightness() <= MIN_BRIGHTNESS;

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
                const int cellId = row * SPRITE_WIDTH + col;
                if (lowest) {
                    // The lowest-brightness colors, twinkling; two in five twinkles flash the glucose
                    // band's color.
                    const int age = twinkleAge(cellId, frame);
                    const bool banded =
                        scramble(cellId + 11, (frame % LOOP_STEPS) / TWINKLE_PERIOD) % 5 < 2;
                    color = age < 0 || age >= 2 ? LOWEST_MANE[band / 2]
                            : banded            ? getLevelColor(level)
                                                : LOWEST_MANE[(band / 2 + 1) % 3];
                } else if (inRange && !mixes) {
                    // The bands hold their colors while single cells twinkle at random: a flash of the
                    // next band's color, then a darker shade.
                    const uint16_t held = IN_RANGE_COLORS[std::min(4, band)];
                    const int age = twinkleAge(cellId, frame);
                    color = age < 0   ? held
                            : age < 2 ? IN_RANGE_COLORS[(std::min(4, band) + 1) % MANE_BANDS]
                                      : shade(held, 0.55f);
                } else if (urgent) {
                    // Past an urgent limit the lights are steady dark stripes.
                    const int dash =
                        ((2 * col - static_cast<int>(frame % 16) + 3 * band) % 16 + 16) % 16;
                    color = dash % 8 < 2 ? 0 : getLevelColor(level);
                } else {
                    // Near a limit, or low or high, lights run along each band toward the tips at
                    // irregular times.
                    const int dash = runDash(band, col, frame);
                    // Every third band holds the neighbouring band's color when the reading is near it.
                    const uint16_t held = mixes && index % 3 == 0 ? neighbour
                                          : inRange              ? IN_RANGE_COLORS[std::min(4, band)]
                                                                 : getMotionColor(level, 1, index, 0);
                    // A shadow trails the light, so it reads as a comet rather than a blink.
                    // The light is the next band's color in range, so the mane never shows white.
                    const uint16_t light = inRange && !(mixes && index % 3 == 0)
                                               ? IN_RANGE_COLORS[(std::min(4, band) + 1) % MANE_BANDS]
                                               : held;
                    color = dash < 2 ? light : dash < 6 ? shade(held, 0.62f) : held;
                }
            } else if (moving) {
                // Back: a new color every five columns slides toward the tips, a column a step. The step
                // count wraps after whole color loops, so the motion never jumps.
                const unsigned long step =
                    flow == MANE_FLOW::BACK ? (SPRITE_WIDTH - 1 - col + frame % 30) / 5 : frame;
                color = mixes && (index + step) % 3 == 0 ? neighbour
                        : inRange                         ? IN_RANGE_COLORS[(index + step) % MANE_BANDS]
                                                          : getMotionColor(level, 1, index, step);
            } else if (inRange) {
                color = IN_RANGE_COLORS[std::min(4, band)];
            } else {
                // One color would merge the bands into a block, so odd bands are darker.
                color = band % 2 == 1 ? shade(getLevelColor(level), 0.62f) : getLevelColor(level);
            }
            // A moving tip drifts a row up or down, into an empty cell only, so the mane looks wispy.
            int drawRow = row;
            if (wisps && !lowest && cell >= 'a') {
                // Each tip picks up, down or stay at its own irregular moments, four steps at a time.
                const uint32_t tip = scramble(col * SPRITE_HEIGHT + row, 0);
                const uint32_t pick = scramble(tip, ((frame + tip % 4) % LOOP_STEPS) / 4);
                const int drift = pick % 4 == 0 ? -1 : pick % 4 == 1 ? 1 : 0;
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
