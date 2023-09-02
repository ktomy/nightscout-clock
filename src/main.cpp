#include <Arduino.h>

#include "SettingsManager.h"
#include "ServerManager.h"
#include "NightscoutManager.h"
#include "globals.h"

void setup() {

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  delay(2000);
  Serial.begin(115200);
  
  SettingsManager.setup();
  if (!SettingsManager.loadSettingsFromFile()) {
    DEBUG_PRINTLN("Error loading config, please re-flash the filesystem");

    ///TODO: Load default settings instead
    
    delay(3000);
    ESP.restart();

  }
  ServerManager.setup();
  NightscoutManager.setup();

  DEBUG_PRINTLN("Setup done");
  delay(3000);
  DEBUG_PRINTLN("Delay done");



}

void loop() {

  if (ServerManager.isConnected)
  {
    ServerManager.tick();
    NightscoutManager.tick();
  }
}
