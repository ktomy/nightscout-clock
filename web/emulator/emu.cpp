// The functions the settings page calls. They drive the firmware's own managers the way main.cpp's
// setup() and loop() do on a connected clock.
#include <ArduinoJson.h>
#include <Button2.h>
#include <FastLED_NeoMatrix.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <emscripten.h>

#include <cmath>
#include <string>

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
std::string emuDrainTones();

// The room at `lux` for the light sensor: a GL5516 photocell on the supply side of a 10 kOhm resistor.
// The firmware averages the sensor, so it reads for 130 s.
static void setRoomLight(double lux) {
    double photocellOhms = std::pow(29634400.0 / std::max(lux, 0.001), 1.0 / 1.6689);
    emuSetLightFraction(1.0 / (photocellOhms / 10000.0 + 1.0));
    for (int i = 0; i < 1300; i++) {
        emuAdvanceMillis(100);
        PeripheryManager.tick();
    }
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void emu_set_clock(double epoch, int tzOffsetSeconds) {
    emuSetClock(epoch, tzOffsetSeconds, true);
}

// setup() without the splash, WiFi and data source. Returns 0 when the config can't be loaded.
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
    setRoomLight(lux);
    return 1;
}

EMSCRIPTEN_KEEPALIVE void emu_set_lux(double lux) { setRoomLight(lux); }

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

// Through PeripheryManager's handlers: 0 left, 1 select, 2 right (clicks), 3 left held, 4 right held,
// 5 select double click.
EMSCRIPTEN_KEEPALIVE void emu_button(int id) {
    switch (id) {
        case 0:
            button_left.emuClick();
            break;
        case 1:
            button_select.emuClick();
            break;
        case 2:
            button_right.emuClick();
            break;
        case 3:
            button_left.emuLongPress();
            break;
        case 4:
            button_right.emuLongPress();
            break;
        case 5:
            button_select.emuDoubleClick();
            break;
    }
}

EMSCRIPTEN_KEEPALIVE int emu_get_face() { return bgDisplayManager.getCurrentFaceId(); }
EMSCRIPTEN_KEEPALIVE void emu_set_face(int id) { bgDisplayManager.setFace(id); }
EMSCRIPTEN_KEEPALIVE int emu_face_count() {
    return static_cast<int>(bgDisplayManager.getFaces().size());
}

// Strip index of panel pixel (x, y).
EMSCRIPTEN_KEEPALIVE int emu_xy(int x, int y) { return matrix->XY(x, y); }

// The frame after the gamma tables and before brightness, RGB bytes in strip order.
EMSCRIPTEN_KEEPALIVE uint8_t* emu_frame() { return reinterpret_cast<uint8_t*>(leds); }

// The last frame sent to the LEDs, RGB bytes in strip order.
EMSCRIPTEN_KEEPALIVE uint8_t* emu_wire() { return FastLED.wire; }

EMSCRIPTEN_KEEPALIVE int emu_brightness() { return FastLED.getBrightness(); }
EMSCRIPTEN_KEEPALIVE int emu_display_on() { return MATRIX_OFF ? 0 : 1; }

// Buzzer changes since the last call: JSON [[millis, hz], ...], 0 hz is silence.
EMSCRIPTEN_KEEPALIVE const char* emu_drain_tones() {
    static std::string json;
    json = emuDrainTones();
    return json.c_str();
}

// What /api/alarm does with a melody.
EMSCRIPTEN_KEEPALIVE void emu_play_rtttl(const char* rtttl) { PeripheryManager.playRTTTLString(rtttl); }
}
