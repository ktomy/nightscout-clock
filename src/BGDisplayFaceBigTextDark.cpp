#include "BGDisplayFaceBigTextDark.h"

void BGDisplayFaceBigTextDark::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showDarkReading(readings.back(), dataIsOld, 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE);
    if (SettingsManager.settings.face_big_text_dark.age_dots) {
        drawAgeDotsUnderArrow(readings.back());
    }
}

// Age dots change every minute, so they redraw on every minute tick.
RenderDecision BGDisplayFaceBigTextDark::getRenderDecision(const RenderContext& ctx) const {
    if (SettingsManager.settings.face_big_text_dark.age_dots) {
        return BGDisplayFaceWithAge::getRenderDecision(ctx);
    }
    return BGDisplayFace::getRenderDecision(ctx);
}
