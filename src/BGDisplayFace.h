#ifndef BGDISPLAYFACE_H
#define BGDISPLAYFACE_H

#include "DisplayManager.h"
#include "BGSource.h"
#include "SettingsManager.h"
#include "enums.h"
#include <list>

class BGDisplayFace {
  public:
    virtual void showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld = false) const = 0;
    virtual void showNoData() const;
};

#endif