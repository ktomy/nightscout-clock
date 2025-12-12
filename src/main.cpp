#include <Arduino.h>
#include <esp32-hal.h>

#include "BGAlarmManager.h"
#include "BGDisplayManager.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "ServerManager.h"
#include "SettingsManager.h"
#include "globals.h"
#include "improv_consume.h"

float apModeHintPosition = MATRIX_WIDTH;  // Start the scrolling right after the screen

void setup() {
    pinMode(15, OUTPUT);
    digitalWrite(15, LOW);
    delay(2000);
    Serial.begin(115200);
    // Serial.setDebugOutput(true);

    DisplayManager.setup();
    SettingsManager.setup();
    if (!SettingsManager.loadSettingsFromFile()) {
        DisplayManager.showFatalError("Error loading software, please reinstall");
    }

    DisplayManager.applySettings();

    DisplayManager.HSVtext(0, 6, String("V " + String(VERSION)).c_str(), true, 0);
    delay(2000);

    PeripheryManager.setup();

    if (PeripheryManager.isButtonSelectPressed()) {
        DEBUG_PRINTLN("Center button pressed, resetting to factory defaults...");
        DisplayManager.scrollColorfulText("Factory reset initiated...");
        DEBUG_PRINTLN("Performing factory reset...");
        SettingsManager.factoryReset();
        DEBUG_PRINTLN("Factory reset done");
    }

    ServerManager.setup();
    bgSourceManager.setup(SettingsManager.settings.bg_source);
    bgDisplayManager.setup();
    bgAlarmManager.setup();
    // PeripheryManager.playRTTTLString(sound_boot);

    DEBUG_PRINTLN("Setup done");
    if (ServerManager.isConnected) {
        String welcomeMessage = "Nightscout clock   " + ServerManager.myIP.toString();
        DisplayManager.scrollColorfulText(welcomeMessage);

        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_WHITE);
        DisplayManager.printText(0, 6, "Connect", TEXT_ALIGNMENT::CENTER, 2);
    }
}

void showJoinAP() {
    SettingsManager.settings.brightness_mode = BRIGHTNES_MODE::MANUAL;
    DisplayManager.setBrightness(70);
    String hint = "Join " + SettingsManager.settings.hostname + " Wi-fi network and go to http://" +
                  ServerManager.myIP.toString() + "/";

    if (apModeHintPosition < -240) {
        apModeHintPosition = 32;
        DisplayManager.clearMatrix();
    }

    DisplayManager.HSVtext(apModeHintPosition, 6, hint.c_str(), true, 1);
    apModeHintPosition -= 0.18;
}

void loop() {
#ifdef DEBUG_MEMORY

    static unsigned long lastMemoryCheck = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastMemoryCheck >= 10000) {  // Check memory every second
        lastMemoryCheck = currentMillis;
        auto freeMemory = ESP.getFreeHeap();
        DEBUG_PRINTLN("Free memory: " + String(freeMemory));
    }
#endif

    if (ServerManager.isConnected) {
        ServerManager.tick();
        bgSourceManager.tick();
        bgDisplayManager.tick();
        bgAlarmManager.tick();

    } else if (ServerManager.isInAPMode) {
        showJoinAP();
    }

    checckForImprovWifiConnection();

    DisplayManager.tick();
    PeripheryManager.tick();
}
