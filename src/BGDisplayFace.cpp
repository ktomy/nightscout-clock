#include "BGDisplayFace.h"

void BGDisplayFace::showNoData() const {
    DisplayManager.clearMatrix();
    DisplayManager.setTextColor(COLOR_GRAY);
    DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
}

bool BGDisplayFace::needsFrequentRefresh() const { return false; }

unsigned long BGDisplayFace::getFrequentRefreshIntervalMs() const { return 5000; }

void BGDisplayFace::onActivate() const {}
