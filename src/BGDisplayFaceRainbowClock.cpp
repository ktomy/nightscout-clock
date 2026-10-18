#include "BGDisplayFaceRainbowClock.h"

#include "ServerManager.h"
#include "globals.h"

namespace {
constexpr unsigned long RAINBOW_STEP_MS = 80;

// Dimmed, a channel below full strength goes dark, so most rainbow hues would light nothing. These
// colors use only fully lit channels and show at every brightness.
const uint16_t DIM_COLORS[6] = {
    static_cast<uint16_t>(DISPLAY_COLOR::RED),   static_cast<uint16_t>(DISPLAY_COLOR::YELLOW),
    static_cast<uint16_t>(DISPLAY_COLOR::GREEN), static_cast<uint16_t>(DISPLAY_COLOR::CYAN),
    static_cast<uint16_t>(DISPLAY_COLOR::BLUE),  static_cast<uint16_t>(DISPLAY_COLOR::MAGENTA),
};
}  // namespace

void BGDisplayFaceRainbowClock::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showTime();
}

void BGDisplayFaceRainbowClock::showNoData() const {
    DisplayManager.clearMatrix();
    showTime();
}

unsigned long BGDisplayFaceRainbowClock::getAnimationStepMillis() const { return RAINBOW_STEP_MS; }

void BGDisplayFaceRainbowClock::showAnimationFrame(
    const std::list<GlucoseReading>& readings, unsigned long frame) const {
    DisplayManager.clearMatrix();
    showTime();
}

// Seconds change on every tick, so every render redraws the whole face.
RenderDecision BGDisplayFaceRainbowClock::getRenderDecision(const RenderContext& ctx) const {
    return RenderDecision::FULL;
}

// The face shows no glucose, so the rainbow keeps moving whatever the data state.
bool BGDisplayFaceRainbowClock::animatesWithoutFreshData() const { return true; }

bool BGDisplayFaceRainbowClock::ticksEverySecond() const { return true; }

bool BGDisplayFaceRainbowClock::suppressesNewAlarms() const { return true; }

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

    uint8_t hueOffset = (millis() / 20) % 256;
    uint8_t textLength = strlen(text);
    // Below an eighth of the brightness range each character steps through the dim colors instead.
    const int rich = MIN_BRIGHTNESS + (MAX_BRIGHTNESS - MIN_BRIGHTNESS) / 8;
    const bool dim = DisplayManager.getBrightness() < rich;
    DisplayManager.setFont(FONT_TYPE::MEDIUM);

    // Center the whole string like Time only, then draw it character by character.
    int16_t x = (MATRIX_WIDTH - DisplayManager.getTextWidth(text, 2)) / 2;

    for (uint8_t i = 0; i < textLength; i++) {
        char ch[2] = {text[i], '\0'};
        uint8_t hue = map(i, 0, max((int)textLength, 1), 0, 255) + hueOffset;
        DisplayManager.setTextColor(
            dim ? DIM_COLORS[hue * 6 / 256] : DisplayManager.hsvToRgb(hue, 255, 255));
        DisplayManager.printText(x, 6, ch, TEXT_ALIGNMENT::LEFT, 2);
        x += DisplayManager.getTextWidth(ch, 2);
    }
}
