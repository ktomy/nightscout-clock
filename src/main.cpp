#include <Arduino.h>

#include "SettingsManager.h"
#include "ServerManager.h"
#include "NightscoutManager.h"
#include "DisplayManager.h"
#include "BGDisplayManager.h"
#include "globals.h"

float apModeHintPosition = 32;

void setup()
{

    pinMode(15, OUTPUT);
    digitalWrite(15, LOW);
    delay(2000);
    Serial.begin(115200);
    //Serial.setDebugOutput(true);

    DisplayManager.setup();
    SettingsManager.setup();
    if (!SettingsManager.loadSettingsFromFile())
    {
        DEBUG_PRINTLN("Error loading config, please re-flash the filesystem");

        /// TODO: Load default settings instead

        delay(3000);
        ESP.restart();
    }

    DisplayManager.HSVtext(2, 6, "Loading", true, 0);

    ServerManager.setup();
    NightscoutManager.setup();
    BGDisplayManager.setup();

    DEBUG_PRINTLN("Setup done");
    float x = 32;
    while (x >= -120)
    {
        DisplayManager.HSVtext(x, 6, ("Nightscout clock   " + ServerManager.myIP.toString()).c_str(), true, 0);
        x -= 0.18;
    }

    DisplayManager.clearMatrix();
    DisplayManager.setTextColor(0x05c0);
}


void showJoinAP()
{
        String hint = "Join " + SettingsManager.settings.hostname + " Wi-fi network and go to http://" + ServerManager.myIP.toString() + "/";

        if (apModeHintPosition < -240)
        {
            apModeHintPosition = 32;
            DisplayManager.clearMatrix();
        }

        DisplayManager.HSVtext(apModeHintPosition, 6, hint.c_str(), true, 1);
        apModeHintPosition -= 0.18;    
}

void loop()
{
    if (ServerManager.isConnected)
    {
        ServerManager.tick();
        NightscoutManager.tick();
        BGDisplayManager.tick();
        if (NightscoutManager.hasNewData(BGDisplayManager.getLastDisplayedGlucoseEpoch()))
        {
            DEBUG_PRINTLN("We have new data");
            BGDisplayManager.showData(NightscoutManager.getInstance().getGlucoseData());
        }
    }
    else if (ServerManager.isInAPMode)
    {
        showJoinAP();
    }

    DisplayManager.tick();
}
