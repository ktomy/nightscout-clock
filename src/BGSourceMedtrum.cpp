#include "BGSourceMedtrum.h"

#include <Arduino.h>

void BGSourceMedtrum::setup() { status = "initialized"; }

std::list<GlucoseReading> BGSourceMedtrum::updateReadings(std::list<GlucoseReading> existingReadings) {
    // Not implemented yet: retrieve readings from Medtrum server
    return existingReadings;
}
