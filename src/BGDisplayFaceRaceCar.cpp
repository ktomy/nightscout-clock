#include "BGDisplayFaceRaceCar.h"

#include "BGDisplayManager.h"
#include "globals.h"

namespace {

const int AREA_WIDTH = 17;
const int AREA_HEIGHT = 8;
const int CAR_X = 7;
const int CAR_Y = 2;

// No green, yellow or red in the car or the road: those are glucose colors.
const uint16_t BODY = COLOR_WHITE;
const uint16_t HELMET = COLOR_MAGENTA;
const uint16_t WHEEL = COLOR_BLUE;
const uint16_t SPOKE = COLOR_CYAN;
const uint16_t ROAD = COLOR_BLUE;

// B body, H the driver's helmet; the nose points at the reading.
const char* const CAR_ART[] = {
    "....HH...",  //
    "BBBBBBB..",  //
    "BBBBBBBBB",  //
};
const int SPEED_LINE_OFFSETS[] = {0, 4, 2, 5};
const int WHEELS[] = {CAR_X + 1, CAR_X + 6};

}  // namespace

unsigned long BGDisplayFaceRaceCar::getAnimationStepMillis() const {
    return getStepMillis(SettingsManager.settings.face_race_car.speed);
}

void BGDisplayFaceRaceCar::drawRace(int sgv, bool dataIsOld, unsigned long frame) const {
    const uint16_t stale = getDataOldColor();
    const BG_LEVEL level =
        dataIsOld ? BG_LEVEL::INVALID : bgDisplayManager.getGlucoseIntervals().getBGLevel(sgv);
    const int quarter = dataIsOld ? 0 : getWarningQuarter(sgv, level);

    DisplayManager.clearMatrixPart(0, 0, AREA_WIDTH, AREA_HEIGHT);
    // Old data draws every other pixel of the race in the old-data color, so it looks faded in any of them.
    auto put = [&](int x, int y, uint16_t color) {
        if (!dataIsOld) {
            DisplayManager.drawPixel(x, y, color);
        } else if ((x + y) % 2 == 0) {
            DisplayManager.drawPixel(x, y, stale);
        }
    };

    // Dashes stream away from the car while the glucose stripes run toward it.
    for (int line = 0; line < 4; line++) {
        for (int x = 0; x < CAR_X; x++) {
            if ((x + frame + SPEED_LINE_OFFSETS[line]) % 7 >= 3) {
                continue;
            }
            put(x, CAR_Y + line, getMotionColor(level, quarter, CAR_X - 1 - x + line, frame));
        }
    }

    for (int x = 0; x < AREA_WIDTH - 1; x++) {
        if ((x + frame) % 4 < 2) {
            put(x, AREA_HEIGHT - 1, ROAD);
        }
    }

    for (int row = 0; row < 3; row++) {
        for (int col = 0; CAR_ART[row][col] != '\0'; col++) {
            const char cell = CAR_ART[row][col];
            if (cell == '.') {
                continue;
            }
            put(CAR_X + col, CAR_Y + row, cell == 'H' ? HELMET : BODY);
        }
    }

    const bool spin = frame % 2 == 1;
    for (int wheel : WHEELS) {
        put(wheel, CAR_Y + 3, spin ? SPOKE : WHEEL);
        put(wheel + 1, CAR_Y + 3, spin ? WHEEL : SPOKE);
    }
}

void BGDisplayFaceRaceCar::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    drawRace(readings.back().sgv, false, frame);
}

void BGDisplayFaceRaceCar::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    const GlucoseReading& lastReading = readings.back();

    // A fresh race is drawn at its current step by the display manager right after this.
    drawRace(lastReading.sgv, dataIsOld, 0);

    showReading(lastReading, MATRIX_WIDTH - 1, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);

    // Age indicator lives to the right of the race.
    drawTimerBlocks(lastReading, 16, 16, 7);
}
