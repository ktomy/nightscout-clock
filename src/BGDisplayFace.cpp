#include "BGDisplayFace.h"

void BGDisplayFace::showNoData() const {
    DisplayManager.clearMatrix();
    DisplayManager.setTextColor(COLOR_GRAY);
    DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
}
