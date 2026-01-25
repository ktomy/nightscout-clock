#include <ArduinoJson.h>
#include <BGAlarmManager.h>
#include <LightDependentResistor.h>
#include <LittleFS.h>
#include <MelodyPlayer/melody_factory.h>
#include <MelodyPlayer/melody_player.h>
#include <PeripheryManager.h>

#include "Adafruit_SHT31.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"

// Pinouts für das ULANZI-Environment
#define BATTERY_PIN 34
#define BUZZER_PIN 15
#define LDR_PIN 35
#define BUTTON_UP_PIN 26
#define BUTTON_DOWN_PIN 14
#define BUTTON_SELECT_PIN 27
#define RESET_PIN 13
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 21

Adafruit_SHT31 sht31;
MelodyPlayer player(BUZZER_PIN, 1, LOW);
Button2 button_left(BUTTON_UP_PIN);
Button2 button_right(BUTTON_DOWN_PIN);
Button2 button_select(BUTTON_SELECT_PIN);

#define USED_PHOTOCELL LightDependentResistor::GL5516
#define PHOTOCELL_SERIES_RESISTOR 10000

LightDependentResistor photocell(LDR_PIN, PHOTOCELL_SERIES_RESISTOR, USED_PHOTOCELL, 10, 10);

int readIndex = 0;
int sampleIndex = 0;
unsigned long previousMillis_BatTempHum = 0;
unsigned long previousMillis_LDR = 0;
const unsigned long interval_BatTempHum = 10000;
const unsigned long interval_LDR = 100;
int total = 0;
unsigned long startTime;

const int LDRReadings = 1000;
int TotalLDRReadings[LDRReadings];
float sampleSum = 0.0;
float sampleAverage = 0.0;
float brightnessPercent = 0.0;
int lastBrightness = 0;

// The getter for the instantiated singleton instance
PeripheryManager_& PeripheryManager_::getInstance() {
    static PeripheryManager_ instance;
    return instance;
}

// Initialize the global shared instance
PeripheryManager_& PeripheryManager = PeripheryManager.getInstance();

void left_button_pressed(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Left button clicked"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.leftButton();
    }
}

void left_button_pressed_long(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Left button pressed long"));
    DisplayManager.leftButtonLong();
}

void right_button_pressed(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Right button clicked"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.rightButton();
    }
}

void right_button_pressed_long(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Right button pressed long"));
    DisplayManager.rightButtonLong();
}

void select_button_pressed(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Select button clicked"));
    bgAlarmManager.snoozeAlarm();
    if (!BLOCK_NAVIGATION) {
        DisplayManager.selectButton();
    }
}

void select_button_pressed_long(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Select button pressed long"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.selectButtonLong();
    }
}

void select_button_double(Button2& /*btn*/) {
    DEBUG_PRINTLN(F("Select button double pressed"));
    if (!BLOCK_NAVIGATION) {
        if (MATRIX_OFF) {
            DisplayManager.setPower(true);
        } else {
            DisplayManager.setPower(false);
        }
    }
}

const bool PeripheryManager_::isButtonSelectPressed() {
    DEBUG_PRINTLN(F("Checking if select button is pressed"));
    return button_select.isPressed();
}

void PeripheryManager_::setup() {
    DEBUG_PRINTLN(F("Setup periphery"));
    startTime = millis();
    pinMode(LDR_PIN, INPUT);

    button_left.setClickHandler(left_button_pressed);
    button_right.setClickHandler(right_button_pressed);

    button_left.setLongClickTime(500);
    button_right.setLongClickTime(500);
    button_left.setLongClickDetectedHandler(left_button_pressed_long);
    button_right.setLongClickDetectedHandler(right_button_pressed_long);
    button_left.setLongClickDetectedRetriggerable(true);
    button_right.setLongClickDetectedRetriggerable(true);

    button_select.setClickHandler(select_button_pressed);
    button_select.setLongClickTime(1000);
    button_select.setLongClickHandler(select_button_pressed_long);
    button_select.setDoubleClickTime(500);
    button_select.setDoubleClickHandler(select_button_double);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    sht31.begin(0x44);
    photocell.setPhotocellPositionOnGround(false);
}

void PeripheryManager_::tick() {
    button_select.loop();
    button_left.loop();
    button_right.loop();

    unsigned long currentMillis_BatTempHum = millis();
    if (currentMillis_BatTempHum - previousMillis_BatTempHum >= interval_BatTempHum) {
        previousMillis_BatTempHum = currentMillis_BatTempHum;
        uint16_t ADCVALUE = analogRead(BATTERY_PIN);
        BATTERY_PERCENT = max(min((int)map(ADCVALUE, 475, 665, 0, 100), 100), 0);
        BATTERY_RAW = ADCVALUE;
        if (SENSOR_READING) {
            sht31.readBoth(&CURRENT_TEMP, &CURRENT_HUM);
            CURRENT_TEMP += TEMP_OFFSET;
            CURRENT_HUM += HUM_OFFSET;
        }
    }

    unsigned long currentMillis_LDR = millis();
    if (currentMillis_LDR - previousMillis_LDR >= interval_LDR) {
        previousMillis_LDR = currentMillis_LDR;
        TotalLDRReadings[sampleIndex] = analogRead(LDR_PIN);

        sampleIndex = (sampleIndex + 1) % LDRReadings;
        sampleSum = 0.0;
        for (int i = 0; i < LDRReadings; i++) {
            sampleSum += TotalLDRReadings[i];
        }
        sampleAverage = sampleSum / (float)LDRReadings;
        LDR_RAW = sampleAverage;
        CURRENT_LUX = (roundf(photocell.getSmoothedLux() * 1000) / 1000);

        if (!MATRIX_OFF && SettingsManager.settings.brightness_mode != BRIGHTNES_MODE::MANUAL) {
            auto resultingBrightness = MIN_BRIGHTNESS;

            switch (SettingsManager.settings.brightness_mode) {
                case BRIGHTNES_MODE::MANUAL: {
                    // Handled above
                    break;
                }
                case BRIGHTNES_MODE::AUTO_LINEAR: {
                    brightnessPercent = sampleAverage / 4095.0 * 100.0 * 4;

#ifdef DEBUG_BRIGHTNESS
                    DEBUG_PRINTF(
                        "LDR: %d, Lux: %.3f, Brightness Percent: %.2f\n", analogRead(LDR_PIN),
                        photocell.getSmoothedLux(), brightnessPercent);
#endif

                    int autoMax = SettingsManager.settings.auto_balanced_max_brightness;
                    resultingBrightness = map(brightnessPercent, 0, 100, MIN_BRIGHTNESS, autoMax);
                    break;
                }
                case BRIGHTNES_MODE::AUTO_DIMMED: {
                    // Use lux (already smoothed) for a more human-perceived response
                    const float lux =
                        CURRENT_LUX;  // e.g., ~0.1 moonlit, 5–50 indoor, 200+ very bright indoor
                    // Knee bends (may need to tweak here depending on individual devices)
                    const float LUX_KNEE_DARK = 1.0f;   // <=1 lux → stay near MIN_BRIGHTNESS
                    const float LUX_KNEE_ROOM = 30.0f;  // ~30 lux = typical indoor
                    const float LUX_MAX_REF = 300.0f;   // treat this as “very bright indoor”; cap above

                    float t;  // 0..1 brightness control

                    if (lux <= LUX_KNEE_DARK) {
                        // In very dark rooms, keep a non-zero, comfortable floor
                        t = 0.0f;
                    } else if (lux < LUX_KNEE_ROOM) {
                        // Between dark and indoor: gentle ramp (gamma slightly > 1 to hold dim
                        // lower-end)
                        float u = (lux - LUX_KNEE_DARK) / (LUX_KNEE_ROOM - LUX_KNEE_DARK);  // 0..1
                        const float gamma_low = 1.6f;
                        t = powf(u, gamma_low) * 0.45f;  // tops at ~45% of the range by indoor
                    } else {
                        // Above indoor, go logarithmic so it gets obviously brighter in bright rooms
                        float v = (lux > LUX_MAX_REF) ? LUX_MAX_REF : lux;
                        float w = log10f(v / LUX_KNEE_ROOM + 1.0f) /
                                  log10f(LUX_MAX_REF / LUX_KNEE_ROOM + 1.0f);  // 0..1
                        t = 0.45f + w * 0.55f;  // fill the remaining 55% up to MAX
                    }

                    // Convert 0..1 → MIN..MAX
                    int autoMax = SettingsManager.settings.auto_balanced_max_brightness;
                    int target = (int)lroundf(MIN_BRIGHTNESS + t * (autoMax - MIN_BRIGHTNESS));

                    // Light smoothing to avoid flicker when people move near the sensor
                    static int prev = -1;
                    if (prev < 0)
                        prev = target;
                    resultingBrightness = (prev * 3 + target) / 4;
                    prev = resultingBrightness;

                    // Safety clamp
                    if (resultingBrightness < (int)MIN_BRIGHTNESS)
                        resultingBrightness = MIN_BRIGHTNESS;
                    if (resultingBrightness > (int)autoMax)
                        resultingBrightness = autoMax;
                    break;
                }
                default:
                    // Unknown mode
                    DEBUG_PRINTLN(
                        "Unsupported brightness mode: " +
                        toString(SettingsManager.settings.brightness_mode));
                    break;
            }

            DisplayManager.setBrightness(resultingBrightness);
        }
    }
}

const char* PeripheryManager_::readUptime() {
    static char
        uptime[25];  // Make the array static to keep it from being destroyed when the function returns
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;
    unsigned long uptimeSeconds = elapsedTime / 1000;
    sprintf(uptime, "%lu", uptimeSeconds);
    return uptime;
}

const void PeripheryManager_::playRTTTLString(String rtttl) {
    static char melodyName[64];
    Melody melody = MelodyFactory.loadRtttlString(rtttl.c_str());
    player.playAsync(melody);
}
