#include "BGDisplayFaceSmiley.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {
constexpr int SMILEY_SIZE = 8;  // 8x8, occupies the full height on the left

// 8x8 monochrome bitmaps (MSB = leftmost pixel), drawn with a single color.
// Only the facial features are lit; the dark pixels stay off, which reads well
// on the LED matrix. Rows go top (y=0) to bottom (y=7).

// Eyes open, mouth curved up (smile).
const uint8_t smiley_happy[] PROGMEM = {
    0b00000000,
    0b00100100,  // eyes
    0b00100100,
    0b00000000,
    0b01000010,  // mouth corners up...
    0b01000010,
    0b00111100,  // ...meeting at the bottom
    0b00000000,
};

// Eyes open, mouth curved down (frown) - the smile flipped vertically.
const uint8_t smiley_sad[] PROGMEM = {
    0b00000000,
    0b00100100,  // eyes
    0b00100100,
    0b00000000,
    0b00111100,  // mouth top in the middle...
    0b01000010,  // ...corners pulling down
    0b01000010,
    0b00000000,
};

// Slanted brows (outer high, inner low) over a frown = angry.
const uint8_t smiley_angry[] PROGMEM = {
    0b00000000,
    0b01000010,  // brow outer corners high
    0b00100100,  // brow inner corners low (slant toward the nose)
    0b00000000,
    0b00111100,  // frown
    0b01000010,
    0b01000010,
    0b00000000,
};

// Eyes open, straight mouth = neutral (used when there is no data).
const uint8_t smiley_neutral[] PROGMEM = {
    0b00000000,
    0b00100100,  // eyes
    0b00100100,
    0b00000000,
    0b00000000,
    0b01111110,  // straight mouth
    0b00000000,
    0b00000000,
};
}  // namespace

void BGDisplayFaceSmiley::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    BG_LEVEL level = bgDisplayManager.getGlucoseIntervals().getBGLevel(lastReading.sgv);

    const uint8_t* bitmap;
    uint16_t color;
    switch (level) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::WARNING_LOW:
            bitmap = smiley_sad;
            color = COLOR_BLUE;
            break;
        case BG_LEVEL::WARNING_HIGH:
        case BG_LEVEL::URGENT_HIGH:
            bitmap = smiley_angry;
            color = COLOR_RED;
            break;
        case BG_LEVEL::NORMAL:
            bitmap = smiley_happy;
            color = COLOR_GREEN;
            break;
        default:
            bitmap = smiley_neutral;
            color = COLOR_GRAY;
            break;
    }

    // Old data is greyed out regardless of the level (matches the other faces).
    if (dataIsOld) {
        color = COLOR_GRAY;
    }

    DisplayManager.clearMatrix(false);

    DisplayManager.drawBitmap(0, 0, bitmap, SMILEY_SIZE, SMILEY_SIZE, color, false);

    // Reading right-aligned in the space next to the smiley, colored by its level.
    showReading(lastReading, MATRIX_WIDTH, 7, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld, false);

    DisplayManager.update();
}

void BGDisplayFaceSmiley::showNoData() const {
    DisplayManager.clearMatrix(false);
    DisplayManager.drawBitmap(0, 0, smiley_neutral, SMILEY_SIZE, SMILEY_SIZE, COLOR_GRAY, false);
    DisplayManager.setTextColor(COLOR_GRAY);
    String noDataText = SettingsManager.settings.bg_units == BG_UNIT::MMOLL ? "--.-" : "---";
    DisplayManager.printText(MATRIX_WIDTH, 7, noDataText.c_str(), TEXT_ALIGNMENT::RIGHT, 2, false);
    DisplayManager.update();
}
