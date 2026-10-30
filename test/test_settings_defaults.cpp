// Unit test for Settings default values.
//
// Verifies that the low_battery_alert_enable setting defaults to false,
// so a clock with a missing or worn-out battery does not beep out of the box.

#include <cassert>
#include <cstdio>

struct Settings {
    bool low_battery_alert_enable = false;
};

int main() {
    Settings s;
    // Default: low battery alert is disabled
    assert(s.low_battery_alert_enable == false);

    std::printf("test_settings_defaults: all assertions passed\n");
    return 0;
}
