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

// True while a reading is past the early-stale threshold but not yet data-is-old.
// Both bounds are checked here so no face can show the milder state on a stale reading.
bool isReadingEarlyStale(const GlucoseReading& reading) {
    if (!SettingsManager.settings.stale_early_enable) {
        return false;
    }

    const int secondsAgo = reading.getSecondsAgo();
    return secondsAgo >= 60 * SettingsManager.settings.stale_early_minutes &&
           secondsAgo <= 60 * SettingsManager.settings.bg_data_too_old_threshold_minutes;
}

uint16_t BGDisplayFace::getEarlyStaleColor() const {
    return static_cast<uint16_t>(SettingsManager.settings.stale_early_color);
}

RenderDecision BGDisplayFace::getRenderDecision(const RenderContext& ctx) const {
    if (ctx.reason == RenderReason::TIME_TICK) {
        // Both thresholds are crossed by time passing, not by new data, so redraw on either.
        if (ctx.dataIsOld != ctx.wasDataOld || ctx.dataIsEarlyStale != ctx.wasDataEarlyStale) {
            return RenderDecision::FULL;
        }
        return RenderDecision::NONE;
    }

    return RenderDecision::FULL;
}

void BGDisplayFace::renderPartial(const RenderContext& ctx) const {}
