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
EasyButton button_left(BUTTON_UP_PIN);
EasyButton button_right(BUTTON_DOWN_PIN);
EasyButton button_select(BUTTON_SELECT_PIN);

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

void left_button_pressed() {
    DEBUG_PRINTLN(F("Left button clicked"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.leftButton();
    }
}

void right_button_pressed() {
    DEBUG_PRINTLN(F("Right button clicked"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.rightButton();
    }
}

void select_button_pressed() {
    DEBUG_PRINTLN(F("Select button clicked"));
    bgAlarmManager.snoozeAlarm();
    if (!BLOCK_NAVIGATION) {
        DisplayManager.selectButton();
    }
}

void select_button_pressed_long() {
    DEBUG_PRINTLN(F("Select button pressed long"));
    if (!BLOCK_NAVIGATION) {
        DisplayManager.selectButtonLong();
    }
}

void select_button_double() {
    DEBUG_PRINTLN(F("Select button double pressed"));
    if (!BLOCK_NAVIGATION) {
        if (MATRIX_OFF) {
            DisplayManager.setPower(true);
        } else {
            DisplayManager.setPower(false);
        }
    }
}

void PeripheryManager_::setup() {
    DEBUG_PRINTLN(F("Setup periphery"));
    startTime = millis();
    pinMode(LDR_PIN, INPUT);
    button_left.begin();
    button_right.begin();
    button_select.begin();

    button_left.onPressed(left_button_pressed);
    button_right.onPressed(right_button_pressed);

    button_select.onPressed(select_button_pressed);
    button_select.onPressedFor(1000, select_button_pressed_long);
    button_select.onSequence(2, 300, select_button_double);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    sht31.begin(0x44);
    photocell.setPhotocellPositionOnGround(false);
}

void PeripheryManager_::tick() {
    button_select.read();
    button_left.read();
    button_right.read();

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
        if (SettingsManager.settings.auto_brightness && !MATRIX_OFF) {
            brightnessPercent = sampleAverage / 4095.0 * 100.0 * 4;
#ifdef DEBUG_BRIGHTNESS
            DEBUG_PRINTF(
                "LDR: %d, Lux: %.3f, Brightness Percent: %.2f\n", analogRead(LDR_PIN),
                photocell.getSmoothedLux(), brightnessPercent);
#endif
            int brightness = map(brightnessPercent, 0, 100, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
            DisplayManager.setBrightness(brightness);
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
