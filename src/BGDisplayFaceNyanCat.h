#ifndef BGDISPLAYFACENYANCAT_H
#define BGDISPLAYFACENYANCAT_H

#include "BGDisplayFaceTextBase.h"

class BGDisplayFaceNyanCat : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    bool needsFrequentRefresh() const override;
    unsigned long getFrequentRefreshIntervalMs() const override;
    void onActivate() const override;

private:
    void drawCat(int16_t x, int16_t y, uint8_t frame) const;
    void drawNyanRainbow(int16_t startX, int16_t endX, uint8_t waveTick) const;
};

#endif
