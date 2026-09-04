#ifndef BGDISPLAYFACEBIGTEXTRAINBOW_H
#define BGDISPLAYFACEBIGTEXTRAINBOW_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceBigTextRainbow : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;

private:
    void showAnimatedReading(const GlucoseReading& reading, bool dataIsOld) const;
};

#endif
