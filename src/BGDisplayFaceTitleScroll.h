#ifndef BGDISPLAYFACETITLESCROLL_H
#define BGDISPLAYFACETITLESCROLL_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"

class BGDisplayFaceTitleScroll : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;
    void onActivate() const override;

private:
    void showTitleTrain(const std::list<GlucoseReading>& readings, bool dataIsOld) const;
};

#endif
