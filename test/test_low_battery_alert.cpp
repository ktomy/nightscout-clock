// Unit test for low-battery alert gating logic.
//
// Replicates the decision logic from src/PeripheryManager.cpp tick():
//   if (BATTERY_PERCENT <= 5) {
//       if (!low_battery_alert_enable) {
//           // disabled: no beep, reset timer
//       } else {
//           if (alertDue && !player.isPlaying()) { beep; }
//       }
//   }

#include <cassert>
#include <cstdio>

// shouldBeep mirrors the firmware's decision to play the low-battery beep.
bool shouldBeep(bool alertEnabled, int batteryPercent, bool alertDue, bool isPlaying) {
    if (batteryPercent > 5)
        return false;
    if (!alertEnabled)
        return false;
    if (!alertDue || isPlaying)
        return false;
    return true;
}

int main() {
    // Battery above 5%: never beep, regardless of setting
    assert(!shouldBeep(true, 6, true, false));
    assert(!shouldBeep(true, 10, true, false));
    assert(!shouldBeep(true, 50, true, false));
    assert(!shouldBeep(false, 10, true, false));
    assert(!shouldBeep(false, 50, true, false));

    // Battery at or below 5%, alert disabled: never beep
    assert(!shouldBeep(false, 5, true, false));
    assert(!shouldBeep(false, 0, true, false));
    assert(!shouldBeep(false, 3, true, false));

    // Battery at or below 5%, alert enabled, due, nothing playing: beep
    assert(shouldBeep(true, 5, true, false));
    assert(shouldBeep(true, 0, true, false));
    assert(shouldBeep(true, 3, true, false));

    // Battery at or below 5%, alert enabled, but not due: no beep
    assert(!shouldBeep(true, 5, false, false));
    assert(!shouldBeep(true, 0, false, false));

    // Battery at or below 5%, alert enabled, due, but already playing: no beep
    assert(!shouldBeep(true, 5, true, true));
    assert(!shouldBeep(true, 0, true, true));

    std::printf("test_low_battery_alert: all assertions passed\n");
    return 0;
}
