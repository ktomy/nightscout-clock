#include "BGDisplayFaceSmiley.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {
constexpr int SMILEY_SIZE = 8;  // 8x8, occupies the full height on the left

// 8x8 monochrome bitmaps (MSB = leftmost pixel). Rows go top (y=0) to bottom
// (y=7). The face is drawn in two passes: first the filled disc in the mood
// color to give a round "background" face, then the features on top in black so
// the eyes and mouth read as dark cutouts (drawBitmap leaves 0-bits untouched,
// so only the lit pixels are punched out).

// Filled round face (the colored background disc).
const uint8_t face_disc[] PROGMEM = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
};

// Eyes + smile (mouth curves up).
const uint8_t features_happy[] PROGMEM = {
    0b00000000,
    0b00000000,
    0b00100100,  // eyes
    0b00000000,
    0b01000010,  // mouth corners up...
    0b00111100,  // ...meeting at the bottom
    0b00000000,
    0b00000000,
};

// Eyes + frown (mouth curves down) - the smile flipped.
const uint8_t features_sad[] PROGMEM = {
    0b00000000,
    0b00000000,
    0b00100100,  // eyes
    0b00000000,
    0b00111100,  // mouth top in the middle...
    0b01000010,  // ...corners pulling down
    0b00000000,
    0b00000000,
};

// Slanted brows over eyes + frown = angry.
const uint8_t features_angry[] PROGMEM = {
    0b00000000,
    0b00000000,
    0b01000010,  // brow outer corners high
    0b00100100,  // brow inner corners low (slant toward the nose) / eyes
    0b00000000,
    0b00111100,  // frown top
    0b01000010,  // frown corners down
    0b00000000,
};

// Eyes + straight mouth = neutral (used when there is no data).
const uint8_t features_neutral[] PROGMEM = {
    0b00000000,
    0b00000000,
    0b00100100,  // eyes
    0b00000000,
    0b00000000,
    0b00111100,  // straight mouth
    0b00000000,
    0b00000000,
};

// Fade an RGB565 color toward black by scaling each channel by FACE_DIM_NUM /
// FACE_DIM_DEN. Used to draw the round face background slightly dimmed so it
// reads as a soft badge behind the crisp cutout features and the reading.
constexpr uint16_t FACE_DIM_NUM = 3;
constexpr uint16_t FACE_DIM_DEN = 5;
uint16_t fadeColor(uint16_t color) {
    uint16_t r = (color >> 11) & 0x1F;
    uint16_t g = (color >> 5) & 0x3F;
    uint16_t b = color & 0x1F;
    r = r * FACE_DIM_NUM / FACE_DIM_DEN;
    g = g * FACE_DIM_NUM / FACE_DIM_DEN;
    b = b * FACE_DIM_NUM / FACE_DIM_DEN;
    return (r << 11) | (g << 5) | b;
}

// Draw the reading in the large font, centered in the region to the right of
// the smiley (x in [SMILEY_SIZE, MATRIX_WIDTH]).
void drawCenteredReadingRight(const String& text) {
    int regionWidth = MATRIX_WIDTH - SMILEY_SIZE;

    // Prefer the large font, but fall back to the narrower MEDIUM font when the
    // value is too wide for the space beside the face (e.g. 4-character mmol/L
    // readings like "22.3"), so it is centered and never clipped off the right
    // edge. This relies on MEDIUM mapping to the compact AwtrixFont in
    // DisplayManager::setFont; if MEDIUM is ever given larger glyphs this width
    // fallback must be revisited.
    DisplayManager.setFont(FONT_TYPE::LARGE);
    int textWidth = (int)DisplayManager.getTextWidth(text.c_str(), 2);
    int baselineY = 7;  // large font (yAdvance 8) fills the full height
    if (textWidth > regionWidth) {
        DisplayManager.setFont(FONT_TYPE::MEDIUM);
        textWidth = (int)DisplayManager.getTextWidth(text.c_str(), 2);
        baselineY = 6;  // MEDIUM (AwtrixFont) sits centered at this baseline
    }

    int x = SMILEY_SIZE + max(0, (regionWidth - textWidth) / 2);
    DisplayManager.printText(x, baselineY, text.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);
}
}  // namespace

void BGDisplayFaceSmiley::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    BG_LEVEL level = bgDisplayManager.getGlucoseIntervals().getBGLevel(lastReading.sgv);

    const uint8_t* features;
    uint16_t faceColor;
    switch (level) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::WARNING_LOW:
            features = features_sad;
            faceColor = COLOR_BLUE;
            break;
        case BG_LEVEL::WARNING_HIGH:
        case BG_LEVEL::URGENT_HIGH:
            features = features_angry;
            faceColor = COLOR_RED;
            break;
        case BG_LEVEL::NORMAL:
            features = features_happy;
            faceColor = COLOR_GREEN;
            break;
        default:
            features = features_neutral;
            faceColor = COLOR_GRAY;
            break;
    }

    // Old data is greyed out regardless of the level (matches the other faces).
    if (dataIsOld) {
        faceColor = COLOR_GRAY;
    }

    DisplayManager.clearMatrix(false);

    // Round face background (slightly faded), then the features punched out in
    // black on top.
    DisplayManager.drawBitmap(0, 0, face_disc, SMILEY_SIZE, SMILEY_SIZE, fadeColor(faceColor), false);
    DisplayManager.drawBitmap(0, 0, features, SMILEY_SIZE, SMILEY_SIZE, COLOR_BLACK, false);

    // Big reading, colored by level, centered beside the face.
    if (dataIsOld) {
        DisplayManager.setTextColor(BG_COLOR_OLD);
    } else {
        SetDisplayColorByBGValue(lastReading);
    }
    drawCenteredReadingRight(getPrintableReading(lastReading.sgv));

    DisplayManager.update();
}

void BGDisplayFaceSmiley::showNoData() const {
    DisplayManager.clearMatrix(false);
    DisplayManager.drawBitmap(0, 0, face_disc, SMILEY_SIZE, SMILEY_SIZE, fadeColor(COLOR_GRAY), false);
    DisplayManager.drawBitmap(0, 0, features_neutral, SMILEY_SIZE, SMILEY_SIZE, COLOR_BLACK, false);
    DisplayManager.setTextColor(COLOR_GRAY);
    String noDataText = SettingsManager.settings.bg_units == BG_UNIT::MMOLL ? "--.-" : "---";
    drawCenteredReadingRight(noDataText);
    DisplayManager.update();
}
