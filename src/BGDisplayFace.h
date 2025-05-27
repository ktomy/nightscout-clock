#ifndef BGDISPLAYFACE_H
#define BGDISPLAYFACE_H

#include <list>

#include "BGSource.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "enums.h"

class BGDisplayFace {
public:
    virtual void showReadings(
        const std::list<GlucoseReading>& readings, bool dataIsOld = false) const = 0;
    virtual void showNoData() const;
};

#endif