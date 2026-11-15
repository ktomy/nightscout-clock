// Stand-in for Button2: keeps PeripheryManager's click handlers so the preview can press the buttons.
#ifndef EMU_BUTTON2_H
#define EMU_BUTTON2_H

#include <Arduino.h>

class Button2 {
public:
    typedef void (*CallbackFunction)(Button2&);

    explicit Button2(uint8_t) {}

    void setClickHandler(CallbackFunction f) { click = f; }
    void setLongClickHandler(CallbackFunction) {}
    void setLongClickDetectedHandler(CallbackFunction) {}
    void setDoubleClickHandler(CallbackFunction) {}
    void setLongClickTime(unsigned int) {}
    void setDoubleClickTime(unsigned int) {}
    void setLongClickDetectedRetriggerable(bool) {}
    void loop() {}
    bool isPressed() const { return false; }

    void emuClick() {
        if (click)
            click(*this);
    }

private:
    CallbackFunction click = nullptr;
};

#endif
