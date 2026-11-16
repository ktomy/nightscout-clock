// Emulator stand-in for the ESP32 core's Ticker: one-shot callbacks due on the emulator's millis(),
// run by Ticker::runDue() from the emulator's loop.
#ifndef EMU_TICKER_H
#define EMU_TICKER_H

#include <Arduino.h>

#include <functional>

class Ticker {
public:
    Ticker();
    ~Ticker();

    template <typename TArg>
    void once(float seconds, void (*callback)(TArg), TArg arg) {
        arm(static_cast<unsigned long>(seconds * 1000), [callback, arg]() { callback(arg); });
    }
    template <typename TArg>
    void once_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
        arm(milliseconds, [callback, arg]() { callback(arg); });
    }
    void once(float seconds, void (*callback)()) {
        arm(static_cast<unsigned long>(seconds * 1000), callback);
    }
    void once_ms(uint32_t milliseconds, void (*callback)()) { arm(milliseconds, callback); }
    void detach() { armed = false; }
    bool active() const { return armed; }

    static void runDue();

private:
    void arm(unsigned long milliseconds, std::function<void()> callback);
    unsigned long due = 0;
    bool armed = false;
    std::function<void()> fn;
};

#endif
