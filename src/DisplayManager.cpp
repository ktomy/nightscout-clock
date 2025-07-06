#include "DisplayManager.h"

#include <FastLED_NeoMatrix.h>

#include <map>

#include "AwtrixFont.h"
#include "BGDisplayManager.h"
#include "ClockFont.h"
#include "MuFont.h"
#include "globals.h"
#include "improv_consume.h"

// The getter for the instantiated singleton instance
DisplayManager_& DisplayManager_::getInstance() {
    static DisplayManager_ instance;
    return instance;
}

ClockFont currentFont = AwtrixFont;

// Initialize the global shared instance
DisplayManager_& DisplayManager = DisplayManager.getInstance();

#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 8
bool UPPERCASE_LETTERS = true;
#define MATRIX_PIN 32

CRGB leds[MATRIX_WIDTH * MATRIX_HEIGHT];

FastLED_NeoMatrix* matrix = new FastLED_NeoMatrix(
    leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);

void DisplayManager_::setFont(FONT_TYPE fontType) {
    switch (fontType) {
        case FONT_TYPE::SMALL:
            currentFont = AwtrixFont;
            break;
        case FONT_TYPE::MEDIUM:
            currentFont = AwtrixFont;
            break;
        case FONT_TYPE::LARGE:
            currentFont = muHeavy8ptBold;
        default:
            break;
    }
    matrix->setFont(&currentFont.font);
}

void setMatrixLayout(int layout) {
    delete matrix;  // Free memory from the current matrix object
    DEBUG_PRINTF("Set matrix layout to %i", layout);
    switch (layout) {
        case 0:
            matrix = new FastLED_NeoMatrix(
                leds, MATRIX_WIDTH, 8,
                NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
            break;
        case 1:
            matrix = new FastLED_NeoMatrix(
                leds, 8, 8, 4, 1,
                NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
            break;
        case 2:
            matrix = new FastLED_NeoMatrix(
                leds, MATRIX_WIDTH, 8,
                NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
            break;
        default:
            break;
    }
}

void DisplayManager_::setup() {
    random16_set_seed(millis());
    FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(leds, MATRIX_WIDTH * MATRIX_HEIGHT);
    setMatrixLayout(0);
    matrix->setRotation(0);
    matrix->begin();
    matrix->setTextWrap(false);
    matrix->setBrightness(70);
    matrix->setFont(&currentFont.font);
}

void DisplayManager_::applySettings() {
    int min_brightness = MIN_BRIGHTNESS;
    int max_brightness = MAX_BRIGHTNESS;
    
    if (SettingsManager.settings.dimmer_mode_enable) {
        min_brightness = DIMMER_MIN_BRIGHTNESS;
        max_brightness = DIMMER_MAX_BRIGHTNESS;
    }
    
    if (SettingsManager.settings.brighter_mode_enable) {
        min_brightness = BRIGHTER_MIN_BRIGHTNESS;
        max_brightness = BRIGHTER_MAX_BRIGHTNESS;
    }
    
    if (!SettingsManager.settings.auto_brightness) {
        DisplayManager.setBrightness(
        map(SettingsManager.settings.brightness_level, 0, 10, min_brightness, max_brightness)
    );
    }

    DEBUG_PRINTLN(
        "Setting brightness to " + String(displayBrightness) +
        " auto brightness: " + String(SettingsManager.settings.auto_brightness) +
        " and brightness level: " + String(SettingsManager.settings.brightness_level));
    DisplayManager.setBrightness(
    map(SettingsManager.settings.brightness_level, 0, 10, min_brightness, max_brightness)
);
}

void DisplayManager_::tick() {}

uint32_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
    CHSV hsv(h, s, v);
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb);
    return ((uint16_t)(rgb.r & 0xF8) << 8) | ((uint16_t)(rgb.g & 0xFC) << 3) | (rgb.b >> 3);
}

float DisplayManager_::getTextWidth(const char* text, byte textCase) {
    float width = 0;
    for (const char* c = text; *c != '\0'; ++c) {
        char current_char = *c;
        if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1) {
            current_char = toupper(current_char);
        }
        if (currentFont.charSizeMap.count(current_char) > 0) {
            width += currentFont.charSizeMap[current_char];
        } else {
            width += 4;
        }
    }
    return width;
}
void DisplayManager_::setTextColor(uint16_t color) { matrix->setTextColor(color); }

void DisplayManager_::clearMatrix() {
    matrix->clear();
    matrix->show();
}

// DisplayManager_::printText(int16_t x, int16_t y, const char *text, TEXT_ALIGNMENT alignment, byte
// textCase) {
// }

void DisplayManager_::printText(
    int16_t x, int16_t y, const char* text, TEXT_ALIGNMENT alignment, byte textCase) {
    if (alignment == TEXT_ALIGNMENT::LEFT) {
        matrix->setCursor(x, y);
    } else if (alignment == TEXT_ALIGNMENT::RIGHT) {
        uint16_t textWidth = getTextWidth(text, textCase);
        int16_t textX = x - textWidth;
        matrix->setCursor(textX, y);
    } else if (alignment == TEXT_ALIGNMENT::CENTER) {
        uint16_t textWidthForCenter = getTextWidth(text, textCase);
        int16_t textXForCenter = ((MATRIX_WIDTH - textWidthForCenter) / 2);
        matrix->setCursor(textXForCenter, y);
    }

    if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1) {
        size_t length = strlen(text);
        char upperText[length + 1];  // +1 for the null terminator

        for (size_t i = 0; i < length; ++i) {
            upperText[i] = toupper(text[i]);
        }

        upperText[length] = '\0';  // Null terminator
        matrix->print(upperText);
    } else {
        matrix->print(text);
    }

    matrix->show();
}

void DisplayManager_::drawBitmap(
    int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
    matrix->setCursor(x, y);
    matrix->drawBitmap(x, y, bitmap, w, h, color);
}

void DisplayManager_::scrollColorfulText(String message) {
    auto finalPosition = -1 * getTextWidth(message.c_str(), 1);

    float x = MATRIX_WIDTH;
    while (x >= finalPosition) {
        checckForImprovWifiConnection();
        DisplayManager.HSVtext(x, 6, (message).c_str(), true, 0);
        x -= 0.18;
    }
}

void DisplayManager_::HSVtext(int16_t x, int16_t y, const char* text, bool clear, byte textCase) {
    if (clear)
        matrix->clear();
    static uint8_t hueOffset = 0;
    uint16_t xpos = 0;
    for (uint16_t i = 0; i < strlen(text); i++) {
        uint8_t hue = map(i, 0, strlen(text), 0, 255) + hueOffset;
        uint32_t textColor = hsvToRgb(hue, 255, 255);
        matrix->setTextColor(textColor);
        const char* myChar = &text[i];

        matrix->setCursor(xpos + x, y);
        if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1) {
            matrix->print((char)toupper(text[i]));
        } else {
            matrix->print(&text[i]);
        }
        char temp_str[2] = {'\0', '\0'};
        temp_str[0] = text[i];
        xpos += getTextWidth(temp_str, textCase);
    }
    hueOffset++;
    if (clear)
        matrix->show();
}

void DisplayManager_::showFatalError(String errorMessage) {
    DEBUG_PRINTF("Fatal error: %s\n", errorMessage.c_str());
    setTextColor(COLOR_GRAY);

    auto startMills = millis();

    while (true) {
        if (millis() - startMills > 16 * 1000 * 10) {
            ESP.restart();
        }

        auto finalPosition = -1 * getTextWidth(errorMessage.c_str(), 1);
        float position = MATRIX_WIDTH;
        while (position > finalPosition) {
            matrix->clear();
            printText(position, 6, errorMessage.c_str(), TEXT_ALIGNMENT::LEFT, 1);
            position -= 0.18;
            checckForImprovWifiConnection();
        }

        clearMatrix();
    }
}

void DisplayManager_::drawPixel(uint8_t x, uint8_t y, uint16_t color, bool updateMatrix) {
    matrix->drawPixel(x, y, color);
    if (updateMatrix) {
        matrix->show();
    }
}

void DisplayManager_::setBrightness(int bri) {
    if (MATRIX_OFF) {
        matrix->setBrightness(0);
    } else {
        matrix->setBrightness(bri);
    }

    matrix->show();
}

void DisplayManager_::setPower(bool state) {
    if (state) {
        MATRIX_OFF = false;
        setBrightness(BRIGHTNESS);
    } else {
        MATRIX_OFF = true;
        // showSleepAnimation();
        setBrightness(0);
    }
}

// cycle to previous face
void DisplayManager_::leftButton() {
    if (bgDisplayManager.getCurrentFaceId() == 0) {
        bgDisplayManager.setFace(bgDisplayManager.getFaces().size() - 1);
    } else {
        bgDisplayManager.setFace(bgDisplayManager.getCurrentFaceId() - 1);
    }
}

// cycle to next face
void DisplayManager_::rightButton() {
    if (bgDisplayManager.getCurrentFaceId() == bgDisplayManager.getFaces().size() - 1) {
        bgDisplayManager.setFace(0);
    } else {
        bgDisplayManager.setFace(bgDisplayManager.getCurrentFaceId() + 1);
    }
}

void DisplayManager_::selectButton() {}
void DisplayManager_::selectButtonLong() {}

void DisplayManager_::update() { matrix->show(); }

void DisplayManager_::clearMatrixPart(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    matrix->fillRect(x, y, width, height, 0);
    matrix->show();
}
