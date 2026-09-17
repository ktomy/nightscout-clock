#ifndef BGDISPLAYFACE_H
#define BGDISPLAYFACE_H

#include <list>

#include "BGSource.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "enums.h"

enum class RenderReason {
    NEW_DATA,
    TIME_TICK,
    FACE_CHANGE,
    FORCED,
};

enum class RenderDecision {
    NONE,
    PARTIAL,
    FULL,
};

struct RenderContext {
    RenderReason reason;
    tm currentTime;
    bool dataIsOld;
    bool wasDataOld;
    const std::list<GlucoseReading>& readings;
};

class BGDisplayFace {
public:
    virtual void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const = 0;
    virtual void showNoData() const;
    virtual RenderDecision getRenderDecision(const RenderContext& ctx) const;
    virtual void renderPartial(const RenderContext& ctx) const;
    // How often a moving face redraws its moving part; 0 for a still face.
    virtual unsigned long getAnimationStepMillis() const { return 0; }
    // Redraws only the moving part of the face for animation step `frame`.
    virtual void showAnimationFrame(
        const std::list<GlucoseReading>& readings, unsigned long frame) const {}

protected:
    // Configurable color for old readings and no-data screens;
    // gray can be invisible at minimum brightness.
    uint16_t getDataOldColor() const;
    // The display color of a glucose range.
    static uint16_t getLevelColor(BG_LEVEL level);
    // Which quarter of the low or high range a reading is in: 0 next to in range, 3 next to urgent.
    static int getWarningQuarter(int sgv, BG_LEVEL level);
    // Seven eighths of the way to white: the lighter stripe of a moving part, lit at the lowest brightness.
    static uint16_t lighten(uint16_t color);
    // The glucose color of pixel `index` of a moving part at step `frame`: a lighter stripe in low or
    // high, urgent color near the urgent limit, and every third pixel dark past an urgent limit.
    uint16_t getMotionColor(BG_LEVEL level, int quarter, int index, unsigned long frame) const;
    static unsigned long getStepMillis(ANIMATION_SPEED speed);
};

#endif
