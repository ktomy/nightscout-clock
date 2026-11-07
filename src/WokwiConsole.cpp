#ifdef WOKWI

#include "WokwiConsole.h"

#include <Arduino.h>
#include <FastLED.h>

#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "ServerManager.h"

extern CRGB leds[];

namespace {

String inputLine;

BGSourceApi* apiSource() {
    if (bgSourceManager.getCurrentSourceType() != BG_SOURCE::API) {
        return nullptr;
    }
    return static_cast<BGSourceApi*>(bgSourceManager.getSource());
}

void addReading(const String& arguments) {
    int sgv = 0;
    int trend = static_cast<int>(BG_TREND::FLAT);
    int minutesAgo = 0;
    if (sscanf(arguments.c_str(), "%d %d %d", &sgv, &trend, &minutesAgo) < 1 || sgv <= 0 || trend < 0 ||
        trend > static_cast<int>(BG_TREND::RATE_OUT_OF_RANGE) || minutesAgo < 0) {
        Serial.println("WOKWI_ERROR usage: bg <sgv> [trend] [minutes_ago]");
        return;
    }
    auto source = apiSource();
    if (source == nullptr) {
        Serial.println("WOKWI_ERROR the data source is not API");
        return;
    }
    auto now = ServerManager.getUtcEpoch();
    if (now < 1700000000) {
        Serial.println("WOKWI_ERROR the time is not synchronized yet");
        return;
    }

    GlucoseReading reading;
    reading.sgv = sgv;
    reading.trend = static_cast<BG_TREND>(trend);
    reading.epoch = now - minutesAgo * 60;
    source->addReadings({reading});
    Serial.printf("WOKWI_OK bg %d trend %d epoch %llu\n", sgv, trend, reading.epoch);
}

void clearReadings() {
    auto source = apiSource();
    if (source == nullptr) {
        Serial.println("WOKWI_ERROR the data source is not API");
        return;
    }
    source->clearReadings();
    Serial.println("WOKWI_OK clear");
}

void printScreen() {
    Serial.print("WOKWI_SCREEN ");
    char hex[7];
    for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
        snprintf(hex, sizeof(hex), "%02x%02x%02x", leds[i].r, leds[i].g, leds[i].b);
        Serial.print(hex);
    }
    Serial.println();
    Serial.println("WOKWI_OK screen");
}

void runCommand(String line) {
    line.trim();
    if (line.isEmpty()) {
        return;
    }
    int space = line.indexOf(' ');
    String command = space < 0 ? line : line.substring(0, space);
    String arguments = space < 0 ? "" : line.substring(space + 1);

    if (command == "bg") {
        addReading(arguments);
    } else if (command == "clear") {
        clearReadings();
    } else if (command == "screen") {
        printScreen();
    } else {
        Serial.println("WOKWI_ERROR unknown command: " + command);
    }
}

}  // namespace

void wokwiConsoleTick() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            runCommand(inputLine);
            inputLine = "";
        } else if (inputLine.length() < 64) {
            inputLine += c;
        }
    }
}

#endif  // WOKWI
