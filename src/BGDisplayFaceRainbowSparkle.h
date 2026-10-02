#ifndef BGDISPLAYFACERAINBOWSPARKLE_H
#define BGDISPLAYFACERAINBOWSPARKLE_H

#include "BGDisplayFaceTextBase.h"

class BGDisplayFaceRainbowSparkle : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;

private:
    void showAnimatedReading(const GlucoseReading& reading, bool dataIsOld) const;
};

#endif
