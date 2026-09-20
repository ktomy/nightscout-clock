#ifndef BGDISPLAYFACETEXTBASE_H
#define BGDISPLAYFACETEXTBASE_H

#include "BGDisplayFace.h"
#include "BGSource.h"

class BGDisplayFaceTextBase : virtual public BGDisplayFace {
public:
    virtual void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const = 0;

protected:
    // Draws the arrow in freshColor, or in the old-data color once the data is old.
    void showTrendArrow(
        const GlucoseReading reading, int16_t x, int16_t y, bool dataIsOld,
        uint16_t freshColor = static_cast<uint16_t>(DISPLAY_COLOR::WHITE)) const;
    void showTrendVerticalLine(int x, BG_TREND trend, bool dataIsOld) const;
    void showReading(
        const GlucoseReading reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment, FONT_TYPE fontType,
        bool isOld = false) const;
    // Prints the reading in the current text color.
    void printReading(
        const GlucoseReading& reading, int16_t x, int16_t y, TEXT_ALIGNMENT alignment,
        FONT_TYPE font) const;
    uint16_t getColorByBGValue(const GlucoseReading& reading) const;
    String getPrintableReading(const int sgv) const;
};

#endif  // BGDISPLAYFACETEXTBASE_H
