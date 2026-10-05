#ifndef BGDISPLAYFACEBIGTEXTWITHAGE_H
#define BGDISPLAYFACEBIGTEXTWITHAGE_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"

// Like BigText, but the trend arrow is moved up one row to make room for
// the age-indicator dots underneath it (one dot per minute, like other faces).
class BGDisplayFaceBigTextWithAge : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
};

#endif  // BGDISPLAYFACEBIGTEXTWITHAGE_H
