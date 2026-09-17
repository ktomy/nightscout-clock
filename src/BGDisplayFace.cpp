#include "BGDisplayFace.h"

#include <algorithm>

#include "globals.h"

uint16_t BGDisplayFace::lighten(uint16_t color) {
    const uint16_t r = (color >> 11) & 0x1F;
    const uint16_t g = (color >> 5) & 0x3F;
    const uint16_t b = color & 0x1F;
    return ((r + 7 * 0x1F) / 8 << 11) | ((g + 7 * 0x3F) / 8 << 5) | ((b + 7 * 0x1F) / 8);
}

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

uint16_t BGDisplayFace::getLevelColor(BG_LEVEL level) {
    switch (level) {
        case BG_LEVEL::URGENT_LOW:
        case BG_LEVEL::URGENT_HIGH:
            return BG_COLOR_URGENT;
        case BG_LEVEL::WARNING_LOW:
        case BG_LEVEL::WARNING_HIGH:
            return BG_COLOR_WARNING;
        default:
            return BG_COLOR_NORMAL;
    }
}

int BGDisplayFace::getWarningQuarter(int sgv, BG_LEVEL level) {
    const auto& settings = SettingsManager.settings;
    const bool low = level == BG_LEVEL::WARNING_LOW;
    const int distance = low ? settings.bg_low_warn_limit - sgv : sgv - settings.bg_high_warn_limit;
    const int width = low ? settings.bg_low_warn_limit - settings.bg_low_urgent_limit
                          : settings.bg_high_urgent_limit - settings.bg_high_warn_limit;
    return width > 0 ? std::min(3, std::max(0, 4 * distance / width)) : 1;
}

uint16_t BGDisplayFace::getMotionColor(
    BG_LEVEL level, int quarter, int index, unsigned long frame) const {
    const unsigned long position = index + frame;
    const bool third = position % 3 == 0;
    BG_LEVEL shown = level;
    bool light = false;
    if (level == BG_LEVEL::URGENT_LOW || level == BG_LEVEL::URGENT_HIGH) {
        if (third) {
            return 0;
        }
    } else if (level == BG_LEVEL::WARNING_LOW || level == BG_LEVEL::WARNING_HIGH) {
        const BG_LEVEL urgent =
            level == BG_LEVEL::WARNING_LOW ? BG_LEVEL::URGENT_LOW : BG_LEVEL::URGENT_HIGH;
        if (quarter <= 1) {
            light = third;
        } else if (quarter == 2) {
            shown = third ? urgent : level;
        } else {
            shown = position % 2 == 0 ? urgent : level;
        }
    }

    const uint16_t color = getLevelColor(shown);
    return light ? lighten(color) : color;
}

unsigned long BGDisplayFace::getStepMillis(ANIMATION_SPEED speed) {
    switch (speed) {
        case ANIMATION_SPEED::CALM:
            return 180;
        case ANIMATION_SPEED::LIVELY:
            return 60;
        default:
            return 110;
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
