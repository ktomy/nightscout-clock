#include "BGDisplayFaceBigText.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigText::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    if (isEarlyStale(readings.back(), dataIsOld)) {
        DisplayManager.setTextColor(
            static_cast<uint16_t>(SettingsManager.settings.face_big_text.early_stale_color));
        printReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE);
    } else {
        showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, dataIsOld);
    }

    // show arrow in the right part of the screen
    showTrendArrow(readings.back(), MATRIX_WIDTH - 5, 1, dataIsOld);
    if (SettingsManager.settings.face_big_text.age_dots) {
        drawAgeDotsUnderArrow(readings.back());
    }
}

// Age dots change every minute; without them, a reading becomes early stale as time passes, so every
// tick inside that window redraws.
RenderDecision BGDisplayFaceBigText::getRenderDecision(const RenderContext& ctx) const {
    if (SettingsManager.settings.face_big_text.age_dots) {
        return BGDisplayFaceWithAge::getRenderDecision(ctx);
    }
    if (ctx.reason == RenderReason::TIME_TICK && !ctx.readings.empty() &&
        isEarlyStale(ctx.readings.back(), ctx.dataIsOld)) {
        return RenderDecision::FULL;
    }

    return BGDisplayFace::getRenderDecision(ctx);
}

// An urgent reading is never shown as late, so it keeps its glucose color.
bool BGDisplayFaceBigText::isEarlyStale(const GlucoseReading& reading, bool dataIsOld) const {
    const auto& bigText = SettingsManager.settings.face_big_text;
    const bool urgent = isUrgent(bgDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv));
    return bigText.early_stale_enabled && !dataIsOld && !urgent &&
           reading.getSecondsAgo() >= 60 * bigText.early_stale_minutes;
}
