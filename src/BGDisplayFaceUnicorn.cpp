#include "BGDisplayFaceUnicorn.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {

// 12x8 sprite, one palette index per pixel (row-major). 0 = transparent.
// 1 = forelock/horn, 2 = body, 3 = eye, 4-7 = mane bands (blue, green, orange, magenta).
// Traced pixel-by-pixel against the reference photo using the pixel-editor mockup.
const uint8_t unicornSprite[12 * 8] PROGMEM = {
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
    0, 1, 1, 0, 0, 4, 5, 6, 7, 0, 0, 0,  //
    0, 0, 1, 1, 4, 4, 4, 5, 6, 7, 0, 0,  //
    0, 0, 0, 2, 2, 2, 2, 4, 5, 6, 7, 0,  //
    2, 2, 2, 2, 3, 2, 2, 4, 5, 6, 7, 0,  //
    2, 2, 2, 2, 2, 2, 2, 4, 5, 6, 7, 0,  //
    0, 2, 2, 2, 2, 2, 2, 4, 5, 6, 7, 0,  //
    0, 0, 0, 2, 2, 2, 2, 2, 4, 5, 6, 7,  //
};

// Palette slot 0 is unused (index 0 means transparent); slots 1-8 map to sprite indices 1-8.
// Traced from the reference photo - see the pixel-editor mockup export (slot 8 unused).
const uint16_t paletteNormal[8] PROGMEM = {
    0xFE87,  // 1 forelock/horn - gold
    0xF79D,  // 2 body - cream white
    0x18C3,  // 3 eye - near black
    0x4D5F,  // 4 mane band - blue
    0x5EAD,  // 5 mane band - green
    0xFCC7,  // 6 mane band - orange
    0xFA78,  // 7 mane band - magenta
    0x9AFE,  // 8 unused
};

const uint16_t paletteWarning[8] PROGMEM = {
    0xFE87, 0xF79D, 0x18C3,
    BG_COLOR_WARNING, BG_COLOR_WARNING, BG_COLOR_WARNING, BG_COLOR_WARNING, BG_COLOR_WARNING,
};

const uint16_t paletteUrgent[8] PROGMEM = {
    0xFE87, 0xF79D, 0x18C3,
    BG_COLOR_URGENT, BG_COLOR_URGENT, BG_COLOR_URGENT, BG_COLOR_URGENT, BG_COLOR_URGENT,
};

const uint16_t paletteStale[8] PROGMEM = {
    0x7B28, 0x94B2, 0x18C3,
    BG_COLOR_OLD, BG_COLOR_OLD, BG_COLOR_OLD, BG_COLOR_OLD, BG_COLOR_OLD,
};

}  // namespace

const uint16_t* BGDisplayFaceUnicorn::getManePalette(BG_LEVEL level, bool dataIsOld) const {
    if (dataIsOld) {
        return paletteStale;
    }

    switch (level) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::URGENT_HIGH:
            return paletteUrgent;
        case BG_LEVEL::WARNING_LOW:
        case BG_LEVEL::WARNING_HIGH:
            return paletteWarning;
        case BG_LEVEL::NORMAL:
        case BG_LEVEL::INVALID:
        default:
            return paletteNormal;
    }
}

void BGDisplayFaceUnicorn::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(lastReading.sgv);

    DisplayManager.drawIndexedSprite(0, 0, unicornSprite, 12, 8, getManePalette(bgLevel, dataIsOld));

    showReading(lastReading, MATRIX_WIDTH - 1, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);

    // Age indicator lives to the right of the sprite.
    drawTimerBlocks(lastReading, 16, 16, 7);
}
