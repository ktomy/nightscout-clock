#include <Arduino.h>

#include "SettingsManager.h"
#include "ServerManager.h"
#include "globals.h"

void setup() {

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  delay(2000);
  Serial.begin(115200);
  SettingsManager.begin();
  ServerManager.begin();
  SettingsManager.loadSettingsFromFile();
  ServerManager.setup();



  DEBUG_PRINTLN("Setup done");





}

void loop() {
  // put your main code here, to run repeatedly:

  if (ServerManager.isConnected)
  {
    ServerManager.tick();
  }
}
