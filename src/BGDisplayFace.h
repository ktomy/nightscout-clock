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
    virtual bool needsFrequentRefresh() const;
    virtual unsigned long getFrequentRefreshIntervalMs() const;
    // Called when this face becomes the active one. Faces are constructed once
    // and reused, so any per-view state that would otherwise leak across a
    // switch (e.g. a cached "content fits / is scrolling" flag) is reset here.
    virtual void onActivate() const;
    virtual RenderDecision getRenderDecision(const RenderContext& ctx) const;
    virtual void renderPartial(const RenderContext& ctx) const;
};

#endif
