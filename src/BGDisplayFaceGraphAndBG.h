// same as BGDisplayFaceGraph.h but with glucose value
#ifndef BGDISPLAYFACEGRAPHANDBG_H
#define BGDISPLAYFACEGRAPHANDBG_H

#include "BGDisplayFaceGraphBase.h"

class BGDisplayFaceGraphAndBG : public BGDisplayFaceGraphBase {
  public:
    void showReadings(const std::list<GlucoseReading> &readings) const override;
    void markDataAsOld() const override;

  private:
    void showLastReading(const GlucoseReading &reading) const;
    // Add any additional private members or functions here
};

#endif // BGDISPLAYFACEGRAPHANDBG_H
