#ifndef DisplayManager_h
#define DisplayManager_h

#include "enums.h"
#include <Arduino.h>

// Global Matrix variables 
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 8

class DisplayManager_ {
  private:
  public:
    static DisplayManager_ &getInstance();
    void setup();
    void applySettings();
    void tick();

    void HSVtext(int16_t x, int16_t y, const char *text, bool clear, byte textCase);
    void printText(int16_t x, int16_t y, const char *text, TEXT_ALIGNMENT alignment, byte textCase);
    void setTextColor(uint16_t color);
    void clearMatrix();
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    void showFatalError(String errorMessage);
    void scrollColorfulText(String message);
    void drawPixel(uint8_t x, uint8_t y, uint16_t color, bool updateMatrix = false);
    void leftButton();
    void rightButton();
    void selectButton();
    void selectButtonLong();
    void setPower(bool power);
    void setBrightness(int bri);
    void update();
    void clearMatrixPart(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
    float getTextWidth(const char *text, byte textCase);
    void setFont(FONT_TYPE fontType);
};

extern DisplayManager_ &DisplayManager;

#endif
