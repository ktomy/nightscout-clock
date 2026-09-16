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
    // Faces that change every second get a TIME_TICK every second instead of every minute.
    virtual bool ticksEverySecond() const;

protected:
    // Configurable color for old readings and no-data screens;
    // gray can be invisible at minimum brightness.
    uint16_t getDataOldColor() const;
};

#endif
