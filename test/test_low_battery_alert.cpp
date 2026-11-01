// Tests the firmware's decision about when to play the low-battery beep.

#include <cassert>
#include <cstdio>

#include "LowBatteryAlert.h"

static const unsigned long INTERVAL = LowBatteryAlert::REPEAT_INTERVAL_MS;
static const unsigned long START = 1000;

static void beepsOnlyWhenEnabledAndLow() {
    LowBatteryAlert alert;
    assert(!alert.shouldBeep(0, false, false, START));
    assert(!alert.shouldBeep(6, true, false, START));
    assert(!alert.shouldBeep(50, true, false, START));
    assert(alert.shouldBeep(5, true, false, START));

    LowBatteryAlert emptyBattery;
    assert(emptyBattery.shouldBeep(0, true, false, START));
}

static void repeatsAtTheInterval() {
    LowBatteryAlert alert;
    assert(alert.shouldBeep(3, true, false, START));
    assert(!alert.shouldBeep(3, true, false, START + 10000));
    assert(!alert.shouldBeep(3, true, false, START + INTERVAL - 1));
    assert(alert.shouldBeep(3, true, false, START + INTERVAL));
}

static void flutteringReadingDoesNotRestartTheInterval() {
    LowBatteryAlert alert;
    assert(alert.shouldBeep(5, true, false, START));
    assert(!alert.shouldBeep(6, true, false, START + 10000));
    assert(!alert.shouldBeep(5, true, false, START + 20000));
    assert(!alert.shouldBeep(9, true, false, START + 30000));
    assert(!alert.shouldBeep(5, true, false, START + 40000));
}

static void recoveryRestartsTheInterval() {
    LowBatteryAlert alert;
    assert(alert.shouldBeep(5, true, false, START));
    assert(!alert.shouldBeep(10, true, false, START + 10000));
    assert(alert.shouldBeep(5, true, false, START + 20000));
}

static void waitsForAPlayingMelody() {
    LowBatteryAlert alert;
    assert(!alert.shouldBeep(5, true, true, START));
    assert(alert.shouldBeep(5, true, false, START + 10000));
}

static void survivesTheMillisRollover() {
    LowBatteryAlert alert;
    const unsigned long nearRollover = static_cast<unsigned long>(-1000);
    assert(alert.shouldBeep(5, true, false, nearRollover));
    assert(!alert.shouldBeep(5, true, false, nearRollover + 2000));
    assert(alert.shouldBeep(5, true, false, nearRollover + INTERVAL));
}

int main() {
    beepsOnlyWhenEnabledAndLow();
    repeatsAtTheInterval();
    flutteringReadingDoesNotRestartTheInterval();
    recoveryRestartsTheInterval();
    waitsForAPlayingMelody();
    survivesTheMillisRollover();
    std::puts("test_low_battery_alert: all assertions passed");
    return 0;
}
