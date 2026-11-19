#include <Arduino.h>
#include <esp32-hal.h>
#include <esp_heap_caps.h>

#include "BGAlarmManager.h"
#include "BGDisplayManager.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "ServerManager.h"
#include "SettingsManager.h"
#include "TlsMemory.h"
#include "globals.h"
#include "improv_consume.h"

#ifdef WOKWI
#include "WokwiConsole.h"
#endif

// The loop runs the data sources' TLS handshakes, and LibreLinkUp's connections request starts one
// deep in its call chain. The ECDSA check in that handshake overflowed the default 8 KB stack.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

float apModeHintPosition = MATRIX_WIDTH;  // Start the scrolling right after the screen

void setup() {
    pinMode(15, OUTPUT);
    digitalWrite(15, LOW);
    delay(2000);
    Serial.begin(115200);
    // Serial.setDebugOutput(true);
    DEBUG_PRINTF("Nightscout clock %s", VERSION);
    TlsMemory::setup();

    DisplayManager.setup();
    SettingsManager.setup();
    if (!SettingsManager.loadSettingsFromFile()) {
        DisplayManager.showFatalError("Error loading software, please reinstall");
    }

    DisplayManager.applySettings();

    DisplayManager.HSVtext(3, 6, String("V " + String(VERSION)).c_str(), true, 0);
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
#ifndef WOKWI  // Scrolling takes minutes in the slower-than-real-time simulation
        String welcomeMessage = "Nightscout clock | To configure go to http://" + ServerManager.myIP.toString() + "/";
        DisplayManager.scrollColorfulText(welcomeMessage);
#endif

        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_WHITE);
        DisplayManager.printText(0, 6, "Connect", TEXT_ALIGNMENT::CENTER, 2);
        DisplayManager.update();
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
    // Once a minute, so a long log shows memory running low or fragmenting over hours. The largest
    // block matters most: a TLS handshake fails when no single block is big enough for it.
    static unsigned long lastMemoryCheck = 0;
    if (millis() - lastMemoryCheck >= 60000) {
        lastMemoryCheck = millis();
        // Only the memory malloc() can use: ESP.getFreeHeap() and MALLOC_CAP_INTERNAL also count about
        // 38 KB of IRAM that takes only 32-bit allocations, so the heap ran out while they showed 40 KB
        // free. More free blocks for the same free memory means a more fragmented heap.
        multi_heap_info_t heap;
        heap_caps_get_info(&heap, MALLOC_CAP_8BIT);
        DEBUG_PRINTF(
            "[mem] free heap %u, largest block %u, lowest free heap %u, free blocks %u, loop stack "
            "never used %u, RSSI %d dBm; %s",
            heap.total_free_bytes, heap.largest_free_block, heap.minimum_free_bytes, heap.free_blocks,
            uxTaskGetStackHighWaterMark(NULL), WiFi.isConnected() ? WiFi.RSSI() : 0,
            TlsMemory::stats().c_str());
    }
    // A new low shows when something takes a lot of memory for a moment, next to the lines around it.
    static size_t lowestSeen = SIZE_MAX;
    size_t lowest = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    if (lowest + 2048 < lowestSeen) {
        DEBUG_PRINTF(
            "[mem] new low: lowest free heap %u, free heap %u", lowest,
            heap_caps_get_free_size(MALLOC_CAP_8BIT));
        lowestSeen = lowest;
    }
#endif

    ServerManager.tick();

    if (ServerManager.isConnected) {
        bgSourceManager.tick();
        TlsMemory::restartIfOutOfMemory();
        bgDisplayManager.tick();
        bgAlarmManager.tick();

    } else if (ServerManager.isInAPMode) {
        showJoinAP();
    }

#ifdef WOKWI
    wokwiConsoleTick();  // Takes the serial input that Improv uses on real hardware
#else
    checckForImprovWifiConnection();
#endif

    DisplayManager.tick();
    PeripheryManager.tick();
}
