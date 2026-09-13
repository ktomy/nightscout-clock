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
    bool dataIsEarlyStale;
    bool wasDataEarlyStale;
    const std::list<GlucoseReading>& readings;
};

// True while a reading is past the early-stale threshold but not yet data-is-old.
// A free function because the render loop needs the same answer the faces do.
bool isReadingEarlyStale(const GlucoseReading& reading);

class BGDisplayFace {
public:
    virtual void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const = 0;
    virtual void showNoData() const;
    virtual RenderDecision getRenderDecision(const RenderContext& ctx) const;
    virtual void renderPartial(const RenderContext& ctx) const;

protected:
    // Configurable color for old readings and no-data screens;
    // gray can be invisible at minimum brightness.
    uint16_t getDataOldColor() const;

    uint16_t getEarlyStaleColor() const;
};

#endif
