#ifndef BGDISPLAYFACE_H
#define BGDISPLAYFACE_H

#include "DisplayManager.h"
#include "BGSource.h"
#include "SettingsManager.h"
#include "enums.h"
#include <list>

class BGDisplayFace {
  public:
    virtual void showReadings(const std::list<GlucoseReading> &readings) const = 0;
    virtual void markDataAsOld() const = 0;
};

#endif