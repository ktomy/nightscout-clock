#include "BGDisplayFaceSimple.h"
#include "BGDisplayManager.h"

void BGDisplayFaceSimple::showReadings(const std::list<GlucoseReading>& readings) const {
    showReading(readings.back());
}

void BGDisplayFaceSimple::markDataAsOld() const {
    
}

    //Glucose trends
    const uint8_t symbol_doubleUp[] PROGMEM = {	
        0x50, 0xF8, 0x50, 0x50, 0x50,
    };
    const uint8_t symbol_singleUp[] PROGMEM = {
        0x20, 0x70, 0xA8, 0x20, 0x20,
    };
    const uint8_t symbol_fortyFiveUp[] PROGMEM = {
        0x38, 0x18, 0x28, 0x40, 0x80,
    };
    const uint8_t symbol_flat[] PROGMEM = {
        0x20, 0x10, 0xF8, 0x10, 0x20,
    };
    const uint8_t symbol_fortyFiveDown[] PROGMEM = {
        0x80, 0x40, 0x28, 0x18, 0x38,
    };
    const uint8_t symbol_singleDown[] PROGMEM = {
        0x20, 0x20, 0xA8, 0x70, 0x20,
    };
    const uint8_t symbol_doubleDown[] PROGMEM = {
        0x50, 0x50, 0x50, 0xF8, 0x50,
    };

    const uint8_t *glucoseTrendSymbols[] = {
        symbol_doubleUp, symbol_singleUp, symbol_fortyFiveUp, symbol_flat, symbol_fortyFiveDown, symbol_singleDown, symbol_doubleDown,
    };


void BGDisplayFaceSimple::showReading(const GlucoseReading reading) const {
    const uint8_t *trendSymbol = glucoseTrendSymbols[4];
    //trendSymbol = glucoseTrendSymbols[3];
    if (reading.trend > 0 && reading.trend < 8) {
        trendSymbol = glucoseTrendSymbols[reading.trend - 1];
    }

    String readingToDisplay = "";
    if (SettingsManager.settings.bgUnit == MGDL) {
        readingToDisplay += String(reading.sgv);
    } else {
        char buffer[10];
        sprintf(buffer, "%.1f", round((float)reading.sgv / 1.8) / 10);
        readingToDisplay += String(buffer);
    }

    DisplayManager.clearMatrix();

    switch(BGDisplayManager.getGlucoseIntervals().getBGLevel(reading.sgv))
    {
        case URGENT_LOW:
        case URGENT_HIGH:
            DisplayManager.setTextColor(0xF800);
            break;
        case WARNING_LOW:
        case WARNING_HIGH:
            DisplayManager.setTextColor(0xFFE0);
            break;
        case NORMAL:
            DisplayManager.setTextColor(0x07E0);
            break;
        default:
            DisplayManager.setTextColor(0xa514);
    }


    DisplayManager.printText(0, 6, readingToDisplay.c_str(), true, 2);
    DisplayManager.drawBitmap(32-5, 1, trendSymbol, 5, 5, 0xFFFF);
}
