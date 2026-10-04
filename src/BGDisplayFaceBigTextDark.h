#ifndef BGDISPLAYFACEBIGTEXTDARK_H
#define BGDISPLAYFACEBIGTEXTDARK_H

#include "BGDisplayFaceSimpleDark.h"

// Simple (dark) with Big text's placement and font, readable across a dark room.
class BGDisplayFaceBigTextDark : public BGDisplayFaceSimpleDark {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
};

#endif
