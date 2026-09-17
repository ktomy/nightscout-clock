// What the stand-in headers declare: time, pins, LittleFS in memory, the LED output, and the parts of
// ServerManager and BGSourceManager the display code calls.
#include <Arduino.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <Wire.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "BGSourceManager.h"
#include "ServerManager.h"

namespace {
double nowEpoch = 0;
int tzOffsetSeconds = 0;
bool timeSynced = true;
double nowMillis = 0;
double lightFraction = 0.5;
uint8_t adcBits = 12;
const uint8_t LDR_PIN = 35;
}  // namespace

void emuSetClock(double epoch, int offsetSeconds, bool synced) {
    nowEpoch = epoch;
    tzOffsetSeconds = offsetSeconds;
    timeSynced = synced;
}

void emuAdvanceMillis(double ms) { nowMillis += ms; }

void emuSetLightFraction(double fraction) {
    lightFraction = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
}

#undef time
extern "C" time_t emu_time(time_t* out) {
    time_t value = static_cast<time_t>(nowEpoch);
    if (out)
        *out = value;
    return value;
}

extern "C" unsigned long millis(void) { return static_cast<unsigned long>(nowMillis); }
extern "C" unsigned long micros(void) { return static_cast<unsigned long>(nowMillis * 1000); }
extern "C" void delay(unsigned long ms) { nowMillis += ms; }
extern "C" void yield(void) {}

// WMath.cpp from the ESP32 Arduino core.
extern "C" long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long run = in_max - in_min;
    if (run == 0) {
        return -1;
    }
    const long rise = out_max - out_min;
    const long delta = x - in_min;
    return (delta * rise) / run + out_min;
}

// newlib has these on the ESP32; the core's stdlib_noniso.c has the long versions.
extern "C" char* ltoa(long value, char* result, int base);
extern "C" char* ultoa(unsigned long value, char* result, int base);
extern "C" char* itoa(int value, char* result, int base) { return ltoa(value, result, base); }
extern "C" char* utoa(unsigned int value, char* result, int base) { return ultoa(value, result, base); }

size_t heap_caps_get_free_size(uint32_t) { return 0; }
size_t heap_caps_get_largest_free_block(uint32_t) { return 0; }

extern "C" void pinMode(uint8_t, uint8_t) {}
extern "C" void digitalWrite(uint8_t, uint8_t) {}
extern "C" int digitalRead(uint8_t) { return 0; }
extern "C" void analogReadResolution(uint8_t bits) { adcBits = bits; }
// The light sensor's divider voltage, at the ADC resolution the firmware selected.
extern "C" uint16_t analogRead(uint8_t pin) {
    if (pin != LDR_PIN)
        return 0;
    return static_cast<uint16_t>(lround(lightFraction * ((1 << adcBits) - 1)));
}

// The buzzer: each change of tone is kept with its time, for the page to play.
namespace {
std::vector<std::pair<double, double>> tones;
double currentTone = 0;

void recordTone(double frequency) {
    if (frequency == currentTone)
        return;
    currentTone = frequency;
    tones.push_back({nowMillis, frequency});
    if (tones.size() > 512)
        tones.erase(tones.begin());
}
}  // namespace

std::string emuDrainTones() {
    std::string json = "[";
    for (size_t i = 0; i < tones.size(); i++) {
        char item[48];
        snprintf(item, sizeof(item), "%s[%.0f,%.1f]", i ? "," : "", tones[i].first, tones[i].second);
        json += item;
    }
    tones.clear();
    return json + "]";
}

extern "C" double ledcSetup(uint8_t, double frequency, uint8_t) { return frequency; }
extern "C" void ledcAttachPin(uint8_t, uint8_t) {}
extern "C" void ledcDetachPin(uint8_t) { recordTone(0); }
extern "C" void ledcWrite(uint8_t, uint32_t duty) {
    if (duty == 0)
        recordTone(0);
}
extern "C" double ledcWriteTone(uint8_t, double frequency) {
    recordTone(frequency);
    return frequency;
}

TwoWire Wire;
fs::FS SPIFFS;

namespace {
std::vector<Ticker*>& tickers() {
    static std::vector<Ticker*> all;
    return all;
}
}  // namespace

Ticker::Ticker() { tickers().push_back(this); }
Ticker::~Ticker() {
    auto& all = tickers();
    all.erase(std::remove(all.begin(), all.end(), this), all.end());
}
void Ticker::arm(unsigned long milliseconds, std::function<void()> callback) {
    fn = callback;
    due = millis() + milliseconds;
    armed = true;
}
void Ticker::runDue() {
    // A callback may re-arm its own ticker, so fire from a copy.
    auto snapshot = tickers();
    for (Ticker* ticker : snapshot) {
        if (ticker->armed && millis() >= ticker->due) {
            ticker->armed = false;
            ticker->fn();
        }
    }
}

EspClass ESP;
void EspClass::restart() { abort(); }

HardwareSerial Serial;

std::map<std::string, std::string>& emuFiles() {
    static std::map<std::string, std::string> files;
    return files;
}

LittleFSFS LittleFS;

// hsv2rgb.cpp (FastLED 3.6.0), non-AVR path.
static void hsv2rgb_raw_C(const CHSV& hsv, CRGB& rgb) {
    const uint8_t HSV_SECTION_3 = 0x40;
    uint8_t value = hsv.val;
    uint8_t saturation = hsv.sat;
    uint8_t invsat = 255 - saturation;
    uint8_t brightness_floor = (value * invsat) / 256;
    uint8_t color_amplitude = value - brightness_floor;
    uint8_t section = hsv.hue / HSV_SECTION_3;
    uint8_t offset = hsv.hue % HSV_SECTION_3;
    uint8_t rampup = offset;
    uint8_t rampdown = (HSV_SECTION_3 - 1) - offset;
    uint8_t rampup_amp_adj = (rampup * color_amplitude) / (256 / 4);
    uint8_t rampdown_amp_adj = (rampdown * color_amplitude) / (256 / 4);
    uint8_t rampup_adj_with_floor = rampup_amp_adj + brightness_floor;
    uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;

    if (section) {
        if (section == 1) {
            rgb.r = brightness_floor;
            rgb.g = rampdown_adj_with_floor;
            rgb.b = rampup_adj_with_floor;
        } else {
            rgb.r = rampup_adj_with_floor;
            rgb.g = brightness_floor;
            rgb.b = rampdown_adj_with_floor;
        }
    } else {
        rgb.r = rampdown_adj_with_floor;
        rgb.g = rampup_adj_with_floor;
        rgb.b = brightness_floor;
    }
}

void hsv2rgb_spectrum(const CHSV& hsv, CRGB& rgb) {
    CHSV hsv2(hsv);
    hsv2.hue = scale8(hsv2.hue, 191);
    hsv2rgb_raw_C(hsv2, rgb);
}

// colorutils.cpp (FastLED 3.6.0).
uint8_t applyGamma_video(uint8_t brightness, float gamma) {
    float orig = (float)brightness / (255.0);
    float adj = pow(orig, gamma) * (255.0);
    uint8_t result = (uint8_t)(adj);
    if ((brightness > 0) && (result == 0)) {
        result = 1;
    }
    return result;
}

CFastLED FastLED;

// CLEDController::showLeds: each byte goes out as scale8(byte, brightness). Dithering is off below
// 100 FPS, which the clock never reaches.
void CFastLED::show() {
    if (leds == nullptr)
        return;
    int count = numLeds < 32 * 8 ? numLeds : 32 * 8;
    for (int i = 0; i < count; i++) {
        for (int channel = 0; channel < 3; channel++) {
            wire[i * 3 + channel] = brightness == 0 ? 0 : scale8(leds[i].raw[channel], brightness);
        }
    }
}

ServerManager_& ServerManager_::getInstance() {
    static ServerManager_ instance;
    return instance;
}
ServerManager_& ServerManager = ServerManager.getInstance();

unsigned long ServerManager_::getUtcEpoch() { return static_cast<unsigned long>(nowEpoch); }

// The device applies the POSIX time zone from the settings; the preview is given a UTC offset.
tm ServerManager_::getTimezonedTime() {
    time_t local = static_cast<time_t>(nowEpoch) + tzOffsetSeconds;
    tm result;
    gmtime_r(&local, &result);
    return result;
}

bool ServerManager_::tryGetTimezonedTime(tm& timeinfo) {
    if (!timeSynced)
        return false;
    timeinfo = getTimezonedTime();
    return true;
}

BGSourceManager_& BGSourceManager_::getInstance() {
    static BGSourceManager_ instance;
    return instance;
}
BGSourceManager_& bgSourceManager = bgSourceManager.getInstance();

bool BGSourceManager_::hasNewData(unsigned long long epochToCompare) {
    auto lastReadingEpoch = readings.size() > 0 ? readings.back().epoch : 0;
    return lastReadingEpoch > epochToCompare;
}

std::list<GlucoseReading> BGSourceManager_::getGlucoseData() { return readings; }
