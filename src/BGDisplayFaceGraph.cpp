#include <Arduino.h>

#include "BGDisplayFaceGraphBase.h"
#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceGraph::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showGraph(0, MATRIX_WIDTH, 180, readings);
}
