// The functions the settings page calls. They drive the firmware's own managers the way main.cpp's
// setup() and loop() do on a connected clock.
#include <ArduinoJson.h>
#include <Button2.h>
#include <FastLED_NeoMatrix.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <emscripten.h>

#include <cmath>

#include "BGAlarmManager.h"
#include "BGDisplayManager.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "SettingsManager.h"
#include "globals.h"

extern FastLED_NeoMatrix* matrix;
extern CRGB leds[];
extern Button2 button_left;
extern Button2 button_right;
extern Button2 button_select;

void emuSetClock(double epoch, int tzOffsetSeconds, bool synced);
void emuAdvanceMillis(double ms);
void emuSetLightFraction(double fraction);

extern "C" {

EMSCRIPTEN_KEEPALIVE void emu_set_clock(double epoch, int tzOffsetSeconds) {
    emuSetClock(epoch, tzOffsetSeconds, true);
}

// setup() without the splash, WiFi and data source, with the room at `lux` for the light sensor.
// Returns 0 when the config can't be loaded.
EMSCRIPTEN_KEEPALIVE int emu_boot(const char* configJson, double lux) {
    emuFiles()[CONFIG_JSON] = configJson;
    DisplayManager.setup();
    SettingsManager.setup();
    if (!SettingsManager.loadSettingsFromFile()) {
        return 0;
    }
    DisplayManager.applySettings();
    PeripheryManager.setup();
    bgDisplayManager.setup();
    bgAlarmManager.setup();

    // GL5516 photocell on the supply side of a 10 kOhm resistor: lux -> the divider's output. The
    // firmware averages the sensor, so let it read for a while.
    double photocellOhms = std::pow(29634400.0 / std::max(lux, 0.001), 1.0 / 1.6689);
    emuSetLightFraction(1.0 / (photocellOhms / 10000.0 + 1.0));
    for (int i = 0; i < 1300; i++) {
        emuAdvanceMillis(100);
        PeripheryManager.tick();
    }
    return 1;
}

// Readings as JSON [[sgv, trend, epoch], ...], oldest first, shown as a data source's new data.
EMSCRIPTEN_KEEPALIVE void emu_show_readings(const char* json) {
    JsonDocument doc;
    deserializeJson(doc, json);
    bgSourceManager.readings.clear();
    for (JsonArray row : doc.as<JsonArray>()) {
        GlucoseReading reading;
        reading.sgv = row[0].as<int>();
        reading.trend = static_cast<BG_TREND>(row[1].as<int>());
        reading.epoch = row[2].as<unsigned long long>();
        bgSourceManager.readings.push_back(reading);
    }
    bgDisplayManager.showData(bgSourceManager.readings);
}

// One pass of loop() after `elapsedMs`.
EMSCRIPTEN_KEEPALIVE void emu_loop(double elapsedMs) {
    emuAdvanceMillis(elapsedMs);
    bgDisplayManager.tick();
    bgAlarmManager.tick();
    DisplayManager.tick();
    PeripheryManager.tick();
    Ticker::runDue();
}

// 0 left, 1 select, 2 right: a click, through PeripheryManager's handlers.
EMSCRIPTEN_KEEPALIVE void emu_button(int id) {
    Button2* buttons[] = {&button_left, &button_select, &button_right};
    if (id >= 0 && id < 3)
        buttons[id]->emuClick();
}

EMSCRIPTEN_KEEPALIVE int emu_get_face() { return bgDisplayManager.getCurrentFaceId(); }
EMSCRIPTEN_KEEPALIVE void emu_set_face(int id) { bgDisplayManager.setFace(id); }

// Strip index of panel pixel (x, y).
EMSCRIPTEN_KEEPALIVE int emu_xy(int x, int y) { return matrix->XY(x, y); }

// The last frame sent to the LEDs, RGB bytes in strip order.
EMSCRIPTEN_KEEPALIVE uint8_t* emu_wire() { return FastLED.wire; }
}
