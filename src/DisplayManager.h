#ifndef DisplayManager_h
#define DisplayManager_h

#include <Arduino.h>

class DisplayManager_
{
private:

public:
    static DisplayManager_ &getInstance();
    void setup();
    void tick();

    void HSVtext(int16_t x, int16_t y, const char *text, bool clear, byte textCase);
    void printText(int16_t x, int16_t y, const char *text, bool centered, byte textCase);
    void setTextColor(uint16_t color);
    void clearMatrix();
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    void showFatalError(String errorMessage);
    void drawPixel(uint8_t x, uint8_t y, uint16_t color);
    void leftButton();
    void rightButton();
    void selectButton();
    void selectButtonLong();
    void setPower(bool power);
    void setBrightness(int bri);

};

extern DisplayManager_ &DisplayManager;
 
#endif