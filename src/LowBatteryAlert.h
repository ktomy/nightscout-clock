#ifndef LowBatteryAlert_h
#define LowBatteryAlert_h

// This header stays free of Arduino dependencies so that test/ can compile it on the host.

// Decides when the optional low-battery beep plays.
class LowBatteryAlert {
public:
    static const int LOW_PERCENT = 5;
    // One percent is only about two ADC counts, so a reading near LOW_PERCENT flutters. The repeat
    // interval starts over only once the battery has clearly recovered.
    static const int RECOVERED_PERCENT = 10;
    static const unsigned long REPEAT_INTERVAL_MS = 2UL * 60UL * 1000UL;

    // Returns true when the beep should play now. A busy player is retried on the next reading, so
    // the beep never cuts off a glucose alarm.
    bool shouldBeep(int batteryPercent, bool enabled, bool playerBusy, unsigned long nowMillis) {
        if (!enabled || batteryPercent >= RECOVERED_PERCENT) {
            hasBeeped = false;
            return false;
        }
        if (batteryPercent > LOW_PERCENT || playerBusy) {
            return false;
        }
        if (hasBeeped && nowMillis - lastBeepMillis < REPEAT_INTERVAL_MS) {
            return false;
        }

        hasBeeped = true;
        lastBeepMillis = nowMillis;
        return true;
    }

private:
    bool hasBeeped = false;
    unsigned long lastBeepMillis = 0;
};

#endif
