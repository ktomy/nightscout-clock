#include "BGDisplayFaceTimeOnly.h"

#include "BGAlarmManager.h"
#include "BGDisplayManager.h"
#include "ServerManager.h"
#include "globals.h"

void BGDisplayFaceTimeOnly::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    if (bgAlarmManager.isAlarmActive() || (!dataIsOld && isUrgent(readings.back()))) {
        BGDisplayFaceClock::showReadings(readings, dataIsOld);
        return;
    }

    showTime();
}

// Without readings nothing is urgent and no alarm can be active, so only the time is shown.
void BGDisplayFaceTimeOnly::showNoData() const {
    DisplayManager.clearMatrix();
    showTime();
}

// Seconds change on every tick, so every render redraws the whole face.
RenderDecision BGDisplayFaceTimeOnly::getRenderDecision(const RenderContext& ctx) const {
    return RenderDecision::FULL;
}

bool BGDisplayFaceTimeOnly::ticksEverySecond() const { return true; }

bool BGDisplayFaceTimeOnly::isUrgent(const GlucoseReading& reading) const {
    auto bgLevel = bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv);
    return bgLevel == BG_LEVEL::URGENT_LOW || bgLevel == BG_LEVEL::URGENT_HIGH;
}

// 24-hour format shows HH:MM:SS; 12-hour format has no room for seconds beside AM/PM.
void BGDisplayFaceTimeOnly::showTime() const {
    tm timeinfo = ServerManager.getTimezonedTime();

    char text[16];
    if (SettingsManager.settings.time_format == TIME_FORMAT::HOURS_12) {
        int hour = timeinfo.tm_hour % 12 == 0 ? 12 : timeinfo.tm_hour % 12;
        snprintf(
            text, sizeof(text), "%d:%02d %s", hour, timeinfo.tm_min,
            timeinfo.tm_hour < 12 ? "AM" : "PM");
    } else {
        snprintf(
            text, sizeof(text), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    DisplayManager.setTextColor(COLOR_WHITE);
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    DisplayManager.printText(0, 6, text, TEXT_ALIGNMENT::CENTER, 2);
}
