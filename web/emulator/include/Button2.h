// Stand-in for Button2: keeps PeripheryManager's handlers so the preview can press and hold buttons.
#ifndef EMU_BUTTON2_H
#define EMU_BUTTON2_H

#include <Arduino.h>

class Button2 {
public:
    typedef void (*CallbackFunction)(Button2&);

    explicit Button2(uint8_t) {}

    void setClickHandler(CallbackFunction f) { click = f; }
    void setLongClickHandler(CallbackFunction f) { longClick = f; }
    void setLongClickDetectedHandler(CallbackFunction f) { longClickDetected = f; }
    void setDoubleClickHandler(CallbackFunction f) { doubleClick = f; }
    void setLongClickTime(unsigned int) {}
    void setDoubleClickTime(unsigned int) {}
    void setLongClickDetectedRetriggerable(bool) {}
    void loop() {}
    bool isPressed() const { return false; }

    void emuClick() {
        if (click)
            click(*this);
    }
    // Button2 reports a hold as detected, then as a long click when it is released.
    void emuLongPress() {
        if (longClickDetected)
            longClickDetected(*this);
        if (longClick)
            longClick(*this);
    }
    void emuDoubleClick() {
        if (doubleClick)
            doubleClick(*this);
    }

private:
    CallbackFunction click = nullptr;
    CallbackFunction longClick = nullptr;
    CallbackFunction longClickDetected = nullptr;
    CallbackFunction doubleClick = nullptr;
};

#endif
