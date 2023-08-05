#ifndef SettingsManager_H
#define  SettingsManager_H

#include "enums.h"
#include <IPAddress.h>

class Settings
{
    public:
        String ssid;
        String password;
        IPAddress ip;
        IPAddress subnet;
        IPAddress gateway;
        IPAddress dns1;
        IPAddress dns2;
        String hostname;
        String nsUrl;
        String nsApiKey;
        BG_UNIT bgUnit;
        int bgLow;
        int bgHigh;

};

class SettingsManager_
{
private:
    SettingsManager_() = default;

public:
    static SettingsManager_ &getInstance();
    void begin();
    void loadSettingsFromFile();
    void saveSettingsToFile();

    Settings settings;
};

extern SettingsManager_ &SettingsManager;




 
#endif