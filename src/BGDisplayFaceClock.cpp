#include "BGDisplayFaceClock.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceClock::showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld) const {

    DisplayManager.clearMatrix();

    showClock();

    showReading(readings.back(), 33, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
}

void BGDisplayFaceClock::showClock() const {
    // Show current time

    struct tm timeinfo;
    getLocalTime(&timeinfo);

    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.printText(0, 6, timeStr, TEXT_ALIGNMENT::LEFT, 2);
}

void BGDisplayFaceClock::showNoData() const {
    DisplayManager.clearMatrix();
    showClock();
}
