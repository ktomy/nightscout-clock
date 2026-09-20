#include "BGDisplayFace.h"

void BGDisplayFace::showNoData() const {
    DisplayManager.clearMatrix();
    // Reset the font because the previous face may have left LARGE selected,
    // which makes "No data" too wide for the panel.
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    DisplayManager.setTextColor(getDataOldColor());
    DisplayManager.printText(0, 6, "No data", TEXT_ALIGNMENT::CENTER, 0);
}

uint16_t BGDisplayFace::getDataOldColor() const {
    return static_cast<uint16_t>(SettingsManager.settings.data_old_color);
}

uint16_t BGDisplayFace::getBandColor(BG_LEVEL level) const {
    const auto& settings = SettingsManager.settings;
    switch (level) {
        case BG_LEVEL::URGENT_LOW:
            return static_cast<uint16_t>(settings.bg_color_urgent_low);
        case BG_LEVEL::WARNING_LOW:
            return static_cast<uint16_t>(settings.bg_color_low);
        case BG_LEVEL::NORMAL:
            return static_cast<uint16_t>(settings.bg_color_normal);
        case BG_LEVEL::WARNING_HIGH:
            return static_cast<uint16_t>(settings.bg_color_high);
        case BG_LEVEL::URGENT_HIGH:
            return static_cast<uint16_t>(settings.bg_color_urgent_high);
        default:
            return static_cast<uint16_t>(DISPLAY_COLOR::GRAY);
    }
}

RenderDecision BGDisplayFace::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason == RenderReason::TIME_TICK) {
        if (ctx.dataIsOld != ctx.wasDataOld) {
            return RenderDecision::FULL;
        }
        return RenderDecision::NONE;
    }

    return RenderDecision::FULL;
}

void BGDisplayFace::renderPartial(const RenderContext& ctx) const {}

bool BGDisplayFace::ticksEverySecond() const { return false; }

bool BGDisplayFace::suppressesNewAlarms() const { return false; }
