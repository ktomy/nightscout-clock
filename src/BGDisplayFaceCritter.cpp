#include "BGDisplayFaceCritter.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {

// 13x8 sprites, one palette index per pixel (row-major). 0 = transparent,
// 1 = body (dynamic level color), 2 = accent, 3 = eye, 4-7 = extra static colors.

const uint8_t spriteCat[13 * 8] PROGMEM = {
    0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 3, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1,
    0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
};

const uint8_t spriteDog[13 * 8] PROGMEM = {
    0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 3, 1, 1, 1, 3, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 2, 2, 1, 1, 0, 0, 0, 0, 1,
    0, 0, 1, 1, 2, 2, 1, 1, 0, 0, 0, 1, 1,
    0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0
};

const uint8_t spriteFrog[13 * 8] PROGMEM = {
    0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 1, 3, 1, 1, 3, 1, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 1, 1, 1, 2, 2, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const uint8_t spriteFox[13 * 8] PROGMEM = {
    0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
    0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 1, 1, 1, 2, 2, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const uint8_t spriteBunny[13 * 8] PROGMEM = {
    0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 0, 1, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 3, 1, 3, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const uint8_t spriteNarwhal[13 * 8] PROGMEM = {
    0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1,
    0, 0, 1, 1, 2, 1, 1, 1, 0, 0, 0, 1, 1,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0
};

const uint8_t spriteWhale[13 * 8] PROGMEM = {
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    0, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,
    0, 0, 1, 1, 2, 2, 2, 2, 1, 1, 0, 1, 1,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1,
    0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const uint8_t spriteMario[13 * 8] PROGMEM = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 3, 3, 1, 1, 3, 3, 1, 1, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 2, 3, 2, 2, 2, 2, 2, 3, 2, 0, 0,
    0, 0, 2, 2, 3, 3, 3, 3, 2, 2, 0, 0, 0,
    0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
};

const uint8_t spriteLuigi[13 * 8] PROGMEM = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 3, 3, 1, 1, 3, 3, 1, 1, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 2, 3, 2, 2, 2, 2, 2, 3, 2, 0, 0,
    0, 0, 2, 2, 3, 3, 3, 3, 2, 2, 0, 0, 0,
    0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
};

const uint8_t spritePeach[13 * 8] PROGMEM = {
    0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0,
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
    0, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0,
    0, 3, 1, 1, 0, 1, 1, 0, 1, 1, 3, 0, 0,
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
    0, 2, 2, 1, 1, 0, 0, 1, 1, 2, 2, 0, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0
};

const uint8_t spriteToad[13 * 8] PROGMEM = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 0, 0,
    0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 0, 2, 3, 2, 2, 2, 3, 2, 0, 0, 0,
    0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0
};

const uint8_t spritePumpkin[13 * 8] PROGMEM = {
    0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 1, 3, 1, 1, 3, 3, 1, 1, 3, 1, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 1, 1, 0,
    0, 0, 1, 1, 1, 3, 1, 1, 3, 1, 1, 1, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
};

const uint8_t spriteGhost[13 * 8] PROGMEM = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 3, 1, 1, 1, 1, 3, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 3, 3, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0
};

const uint8_t spriteWitch[13 * 8] PROGMEM = {
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 2, 3, 2, 2, 2, 3, 2, 0, 0, 0, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const uint8_t spriteTurkey[13 * 8] PROGMEM = {
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 3, 1, 3, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 4, 5, 6, 4, 5, 6, 4, 5, 6, 4, 0, 0,
    4, 4, 5, 5, 6, 6, 6, 5, 5, 4, 4, 0, 0
};

const uint8_t spriteButterfly[13 * 8] PROGMEM = {
    0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

}  // namespace

BGDisplayFaceCritter::BGDisplayFaceCritter(CritterId id) : critterId(id) {}

const uint8_t* BGDisplayFaceCritter::getSprite() const {
    switch (critterId) {
        case CritterId::CAT: return spriteCat;
        case CritterId::DOG: return spriteDog;
        case CritterId::FROG: return spriteFrog;
        case CritterId::FOX: return spriteFox;
        case CritterId::BUNNY: return spriteBunny;
        case CritterId::NARWHAL: return spriteNarwhal;
        case CritterId::WHALE: return spriteWhale;
        case CritterId::MARIO: return spriteMario;
        case CritterId::LUIGI: return spriteLuigi;
        case CritterId::PEACH: return spritePeach;
        case CritterId::TOAD: return spriteToad;
        case CritterId::PUMPKIN: return spritePumpkin;
        case CritterId::GHOST: return spriteGhost;
        case CritterId::WITCH: return spriteWitch;
        case CritterId::TURKEY: return spriteTurkey;
        case CritterId::BUTTERFLY: return spriteButterfly;
        default: return spriteCat;
    }
}

// Palette base colors (body is overridden dynamically by BG level)
struct CritterColors {
    uint16_t body;  // natural body color (for NORMAL level)
    uint16_t accent;
    uint16_t eye;
    uint16_t c4;
    uint16_t c5;
    uint16_t c6;
    uint16_t c7;
};

static const CritterColors critterColors[] PROGMEM = {
    {0xFC00, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Cat
    {0xD343, 0xFD20, 0x0000, 0xFD20, 0xFD20, 0xFD20, 0xFD20},  // Dog
    {0x07E0, 0x87F0, 0x0000, 0x87F0, 0x87F0, 0x87F0, 0x87F0},  // Frog
    {0xFC00, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Fox
    {0xFFFF, 0xF81F, 0x0000, 0xF81F, 0xF81F, 0xF81F, 0xF81F},  // Bunny
    {0x94B2, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Narwhal
    {0x439F, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Whale
    {0xF800, 0xFD20, 0x0000, 0xFD20, 0xFD20, 0xFD20, 0xFD20},  // Mario
    {0x07E0, 0xFD20, 0x0000, 0xFD20, 0xFD20, 0xFD20, 0xFD20},  // Luigi
    {0xFD20, 0xFCF4, 0xFFE0, 0xFCF4, 0xFCF4, 0xFCF4, 0xFCF4},  // Peach
    {0xF800, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Toad
    {0xFC00, 0x07E0, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0},  // Pumpkin
    {0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},  // Ghost
    {0xA81F, 0x07E0, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0},  // Witch
    {0xD343, 0xF800, 0x0000, 0xF800, 0xFC00, 0xFFE0, 0xF800},  // Turkey
    {0xFC9F, 0x8410, 0x0000, 0x8410, 0x8410, 0x8410, 0x8410},  // Butterfly
};
const uint16_t* BGDisplayFaceCritter::getPalette(BG_LEVEL level, bool dataIsOld) const {
    // Static to avoid stack allocation on each frame. RAM-based (not PROGMEM)
    // because the body color is computed dynamically from the glucose level.
    static uint16_t palette[8];
    uint8_t idx = static_cast<uint8_t>(critterId);

    CritterColors cc;
    memcpy_P(&cc, &critterColors[idx], sizeof(CritterColors));
    uint16_t body;
    if (dataIsOld) {
        body = getDataOldColor();
    } else {
        switch (level) {
            case BG_LEVEL::URGENT_LOW:
            case BG_LEVEL::URGENT_HIGH:
                body = 0xF800;  // red
                break;
            case BG_LEVEL::WARNING_LOW:
            case BG_LEVEL::WARNING_HIGH:
                body = 0xFFE0;  // yellow
                break;
            case BG_LEVEL::NORMAL:
            case BG_LEVEL::INVALID:
            default:
                body = cc.body;
                break;
        }
    }
    palette[0] = 0x0000;  // transparent (unused)
    palette[1] = body;
    palette[2] = dataIsOld ? getDataOldColor() : cc.accent;
    palette[3] = cc.eye;
    palette[4] = dataIsOld ? getDataOldColor() : cc.c4;
    palette[5] = dataIsOld ? getDataOldColor() : cc.c5;
    palette[6] = dataIsOld ? getDataOldColor() : cc.c6;
    palette[7] = dataIsOld ? getDataOldColor() : cc.c7;
    return palette;
}

void BGDisplayFaceCritter::drawSprite(const uint8_t* sprite, const uint16_t* palette) const {
    // drawIndexedSprite requires a PROGMEM palette; our palette is computed
    // in RAM, so draw manually.
    for (int16_t row = 0; row < 8; row++) {
        for (int16_t col = 0; col < 13; col++) {
            uint8_t paletteIndex = pgm_read_byte(&sprite[row * 13 + col]);
            if (paletteIndex == 0) {
                continue;  // transparent
            }
            DisplayManager.drawPixel(col, row, palette[paletteIndex], false);
        }
    }
}

void BGDisplayFaceCritter::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(lastReading.sgv);

    drawSprite(getSprite(), getPalette(bgLevel, dataIsOld));
    // Trend arrow in the gap between the sprite and the reading (white, like standard faces).
    showTrendArrow(lastReading, 14, 2, dataIsOld);
    showReading(lastReading, MATRIX_WIDTH - 1, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    drawTimerBlocks(lastReading, 16, 16, 7);
}

void BGDisplayFaceCritter::showNoData() const {
    DisplayManager.clearMatrix();
    // Show the critter in stale gray with no reading.
    drawSprite(getSprite(), getPalette(BG_LEVEL::INVALID, true));
    String noData = "---";
    if (SettingsManager.settings.bg_units == BG_UNIT::MMOLL) {
        noData = "--.-";
    }
    DisplayManager.setTextColor(getDataOldColor());
    DisplayManager.printText(33, 6, noData.c_str(), TEXT_ALIGNMENT::RIGHT, 2);
}
