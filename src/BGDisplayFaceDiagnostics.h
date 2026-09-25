#ifndef BGDISPLAYFACEDIAGNOSTICS_H
#define BGDISPLAYFACEDIAGNOSTICS_H

#include "BGDisplayFaceTextBase.h"

class BGDisplayFaceDiagnostics : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;
    void onActivate() const override;

private:
    void showDateTimePage(const std::list<GlucoseReading>& readings, bool dataIsOld) const;
    int getScrollX(int contentWidth) const;
    const String& formatDateTime() const;

    // Cache for the date/time string so the 60fps scroll loop does not call the
    // relatively expensive (and potentially blocking) getLocalTime() on every
    // frame, and does not reallocate the String while the minute is unchanged.
    mutable unsigned long lastTimeCheckMillis = 0;
    mutable int cachedMinuteKey = -1;
    mutable String cachedDateTime;

    // True while the content is wider than the matrix and therefore scrolling.
    // Updated in getScrollX and read by needsFrequentRefresh so a static,
    // centered frame does not keep redrawing at the scroll frame rate. Starts
    // true so the first render happens at the frequent rate and can measure.
    mutable bool contentScrolls = true;
};

#endif
