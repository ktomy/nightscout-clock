#include "BGDisplayFaceRainbowClock.h"

#include "ServerManager.h"
#include "globals.h"

namespace {
constexpr unsigned long RAINBOW_REFRESH_INTERVAL_MS = 80;
}

void BGDisplayFaceRainbowClock::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showTime();
}

void BGDisplayFaceRainbowClock::showNoData() const {
    DisplayManager.clearMatrix();
    showTime();
}

bool BGDisplayFaceRainbowClock::needsFrequentRefresh() const { return true; }

unsigned long BGDisplayFaceRainbowClock::getFrequentRefreshIntervalMs() const {
    return RAINBOW_REFRESH_INTERVAL_MS;
}

// Seconds change on every tick, so every render redraws the whole face.
RenderDecision BGDisplayFaceRainbowClock::getRenderDecision(const RenderContext& ctx) const {
    return RenderDecision::FULL;
}

bool BGDisplayFaceRainbowClock::ticksEverySecond() const { return true; }

// 24-hour format shows HH:MM:SS; 12-hour format has no room for seconds beside AM/PM.
void BGDisplayFaceRainbowClock::showTime() const {
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

    uint8_t hueOffset = (millis() / 20) % 255;
    uint8_t textLength = strlen(text);
    DisplayManager.setFont(FONT_TYPE::MEDIUM);

    // Center the whole string like Time only, then draw it character by character.
    int16_t x = (MATRIX_WIDTH - DisplayManager.getTextWidth(text, 2)) / 2;

    for (uint8_t i = 0; i < textLength; i++) {
        char ch[2] = {text[i], '\0'};
        uint8_t hue = map(i, 0, max((int)textLength, 1), 0, 255) + hueOffset;
        DisplayManager.setTextColor(DisplayManager.hsvToRgb565(hue));
        DisplayManager.printText(x, 6, ch, TEXT_ALIGNMENT::LEFT, 2, false);
        x += DisplayManager.getTextWidth(ch, 2);
    }
}
