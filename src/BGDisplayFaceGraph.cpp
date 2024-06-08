#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayManager.h"
#include "globals.h"
#include <Arduino.h>

void BGDisplayFaceGraph::showReadings(const std::list<GlucoseReading> &readings, bool dataIsOld) const {
    showGraph(0, 32, 180, readings);
}
