#include "DisplayManager.h"
#include "globals.h"
#include "AwtrixFont.h"
#include <FastLED_NeoMatrix.h>
#include <map>


// The getter for the instantiated singleton instance
DisplayManager_ &DisplayManager_::getInstance()
{
    static DisplayManager_ instance;
    return instance;
}

// Initialize the global shared instance
DisplayManager_ &DisplayManager = DisplayManager.getInstance();


#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 8
bool UPPERCASE_LETTERS = true;
#define MATRIX_PIN 32

CRGB leds[MATRIX_WIDTH * MATRIX_HEIGHT];

std::map<char, uint16_t> CharMap = {
    {32, 2}, {33, 2}, {34, 4}, {35, 4}, {36, 4}, {37, 4}, {38, 4}, {39, 2}, {40, 3}, {41, 3}, {42, 4}, {43, 4}, {44, 3}, {45, 4}, {46, 2}, {47, 4}, {48, 4}, {49, 4}, {50, 4}, {51, 4}, {52, 4}, {53, 4}, {54, 4}, {55, 4}, {56, 4}, {57, 4}, {58, 2}, {59, 3}, {60, 4}, {61, 4}, {62, 4}, {63, 4}, {64, 4}, {65, 4}, {66, 4}, {67, 4}, {68, 4}, {69, 4}, {70, 4}, {71, 4}, {72, 4}, {73, 2}, {74, 4}, {75, 4}, {76, 4}, {77, 6}, {78, 5}, {79, 4}, {80, 4}, {81, 5}, {82, 4}, {83, 4}, {84, 4}, {85, 4}, {86, 4}, {87, 6}, {88, 4}, {89, 4}, {90, 4}, {91, 4}, {92, 4}, {93, 4}, {94, 4}, {95, 4}, {96, 3}, {97, 4}, {98, 4}, {99, 4}, {100, 4}, {101, 4}, {102, 4}, {103, 4}, {104, 4}, {105, 2}, {106, 4}, {107, 4}, {108, 4}, {109, 4}, {110, 4}, {111, 4}, {112, 4}, {113, 4}, {114, 4}, {115, 4}, {116, 4}, {117, 4}, {118, 4}, {119, 4}, {120, 4}, {121, 4}, {122, 4}, {123, 4}, {124, 2}, {125, 4}, {126, 4}, {161, 2}, {162, 4}, {163, 4}, {164, 4}, {165, 4}, {166, 2}, {167, 4}, {168, 4}, {169, 4}, {170, 4}, {171, 3}, {172, 4}, {173, 3}, {174, 4}, {175, 4}, {176, 3}, {177, 4}, {178, 4}, {179, 4}, {180, 3}, {181, 4}, {182, 4}, {183, 4}, {184, 4}, {185, 2}, {186, 4}, {187, 3}, {188, 4}, {189, 4}, {190, 4}, {191, 4}, {192, 4}, {193, 4}, {194, 4}, {195, 4}, {196, 4}, {197, 4}, {198, 4}, {199, 4}, {200, 4}, {201, 4}, {202, 4}, {203, 4}, {204, 4}, {205, 4}, {206, 4}, {207, 4}, {208, 4}, {209, 4}, {210, 4}, {211, 4}, {212, 4}, {213, 4}, {214, 4}, {215, 4}, {216, 4}, {217, 4}, {218, 4}, {219, 4}, {220, 4}, {221, 4}, {222, 4}, {223, 4}, {224, 4}, {225, 4}, {226, 4}, {227, 4}, {228, 4}, {229, 4}, {230, 4}, {231, 4}, {232, 4}, {233, 4}, {234, 4}, {235, 4}, {236, 3}, {237, 3}, {238, 4}, {239, 4}, {240, 4}, {241, 4}, {242, 4}, {243, 4}, {244, 4}, {245, 4}, {246, 4}, {247, 4}, {248, 4}, {249, 4}, {250, 4}, {251, 4}, {252, 4}, {253, 4}, {254, 4}, {255, 4}, {285, 2}, {338, 4}, {339, 4}, {352, 4}, {353, 4}, {376, 4}, {381, 4}, {382, 4}, {3748, 2}, {5024, 2}, {8226, 2}, {8230, 4}, {8364, 4}, {65533, 4}};


FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);


void setMatrixLayout(int layout)
{
    delete matrix; // Free memory from the current matrix object
    DEBUG_PRINTF("Set matrix layout to %i", layout);
    switch (layout)
    {
    case 0:
        matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
        break;
    case 1:
        matrix = new FastLED_NeoMatrix(leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
        break;
    case 2:
        matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
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
    matrix->setFont(&AwtrixFont);    
}

void DisplayManager_::tick() {

}

uint32_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v)
{
    CHSV hsv(h, s, v);
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb);
    return ((uint16_t)(rgb.r & 0xF8) << 8) |
           ((uint16_t)(rgb.g & 0xFC) << 3) |
           (rgb.b >> 3);
}

float getTextWidth(const char *text, byte textCase)
{
    float width = 0;
    for (const char *c = text; *c != '\0'; ++c)
    {
        char current_char = *c;
        if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1)
        {
            current_char = toupper(current_char);
        }
        if (CharMap.count(current_char) > 0)
        {
            width += CharMap[current_char];
        }
        else
        {
            width += 4;
        }
    }
    return width;
}
void DisplayManager_::setTextColor(uint16_t color)
{
    matrix->setTextColor(color);
}

void DisplayManager_::clearMatrix()
{
    matrix->clear();
    matrix->show();
}

void DisplayManager_::printText(int16_t x, int16_t y, const char *text, bool centered, byte textCase)
{

    if (centered)
    {
        uint16_t textWidth = getTextWidth(text, textCase);
        int16_t textX = ((32 - textWidth) / 2);
        matrix->setCursor(textX, y);
    }
    else
    {
        matrix->setCursor(x, y);
    }

    if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1)
    {
        size_t length = strlen(text);
        char upperText[length + 1]; // +1 for the null terminator

        for (size_t i = 0; i < length; ++i)
        {
            upperText[i] = toupper(text[i]);
        }

        upperText[length] = '\0'; // Null terminator
        matrix->print(upperText);
    }
    else
    {
        matrix->print(text);
    }

    matrix->show();
}

void DisplayManager_::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h,uint16_t color) {
    matrix->setCursor(x, y);
    matrix->drawBitmap(x, y, bitmap, w, h, color);
    matrix->show();
}


void DisplayManager_::HSVtext(int16_t x, int16_t y, const char *text, bool clear, byte textCase)
{
    if (clear)
        matrix->clear();
    static uint8_t hueOffset = 0;
    uint16_t xpos = 0;
    for (uint16_t i = 0; i < strlen(text); i++)
    {
        uint8_t hue = map(i, 0, strlen(text), 0, 255) + hueOffset;
        uint32_t textColor = hsvToRgb(hue, 255, 255);
        matrix->setTextColor(textColor);
        const char *myChar = &text[i];

        matrix->setCursor(xpos + x, y);
        if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1)
        {
            matrix->print((char)toupper(text[i]));
        }
        else
        {
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