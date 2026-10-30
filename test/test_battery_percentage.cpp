// Unit test for battery percentage calculation logic.
//
// Replicates the calculation from src/PeripheryManager.cpp:
//     BATTERY_PERCENT = max(min(map(ADCVALUE, 475, 665, 0, 100), 100), 0);
//
// Arduino map(): (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

#include <algorithm>
#include <cassert>
#include <cstdio>

int calculateBatteryPercent(int adcValue) {
    int mapped = (adcValue - 475) * (100 - 0) / (665 - 475) + 0;
    return std::max(std::min(mapped, 100), 0);
}

int main() {
    // Boundary of the calibrated range → 0%
    assert(calculateBatteryPercent(475) == 0);
    // Boundary of the calibrated range → 100%
    assert(calculateBatteryPercent(665) == 100);
    // Midpoint of the calibrated range → 50%
    assert(calculateBatteryPercent(570) == 50);
    // Below range (worn-out / removed battery reads ~0) → clamped to 0%
    assert(calculateBatteryPercent(0) == 0);
    assert(calculateBatteryPercent(300) == 0);
    assert(calculateBatteryPercent(474) == 0);
    // Above range → clamped to 100%
    assert(calculateBatteryPercent(666) == 100);
    assert(calculateBatteryPercent(1023) == 100);

    std::printf("test_battery_percentage: all assertions passed\n");
    return 0;
}
