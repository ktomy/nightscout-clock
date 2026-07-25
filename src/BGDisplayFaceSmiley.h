#ifndef BGDISPLAYFACESMILEY_H
#define BGDISPLAYFACESMILEY_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

// A face that shows an 8x8 smiley reflecting the current glucose level:
//   - happy  when the reading is in range (between the low and high limits)
//   - sad    when it is below the low limit
//   - angry  when it is above the high limit
// The numeric reading is drawn next to the face.
class BGDisplayFaceSmiley : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
};

#endif  // BGDISPLAYFACESMILEY_H
