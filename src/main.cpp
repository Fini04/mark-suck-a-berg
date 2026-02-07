const int DEBUG = true;

#include <Arduino.h>
#include <esp_types.h>
#include <WiFi.h>
#include <esp_bt.h>
#include "soc/rtc_cntl_reg.h"

#include "engine/engine.hpp"
#include "axisconfigs.hpp"

/*
   |-----------------------------------------------------------------|
   | Hiermit entschuldige ich mich schon einmal im Voraus bei allen, |
   | die dieses ungetüm an Programcode lesen müssen.                 |
   |-----------------------------------------------------------------|
*/

#pragma region INTERRUPTS
volatile unsigned long lastInterruptTimeXHome = 0;
void IRAM_ATTR engine1ValidHome() {
  unsigned long now = millis();
  if (now - lastInterruptTimeXHome > 200) { // 100 ms
    if (digitalRead(axisX.config.pinInputHome) == LOW) {
    axisX.validHome = true;
    }
    lastInterruptTimeXHome = now;
  }
}

volatile unsigned long lastInterruptTimeXStep = 0;
void IRAM_ATTR engine1StepMade() {
  unsigned long now = millis();
  if (now - lastInterruptTimeXStep > 200) { // 100 ms
    if (axisX.steps != 0)
    {
      axisX.steps--;
    }
    lastInterruptTimeXStep = now;
  }
}

volatile unsigned long lastInterruptTimeYHome = 0;
void IRAM_ATTR engine2ValidHome() {
  unsigned long now = millis();
  if (now - lastInterruptTimeYHome > 200) { // 100 ms
    if (digitalRead(axisY.config.pinInputHome) == LOW) {
    axisY.validHome = true;
    }
    lastInterruptTimeYHome = now;
  }
}

volatile unsigned long lastInterruptTimeYStep = 0;
void IRAM_ATTR engine2StepMade() {
  unsigned long now = millis();
  if (now - lastInterruptTimeYStep > 200) { // 100 ms
    if (axisY.steps != 0)
    {
      axisY.steps--;
    }
    lastInterruptTimeYStep = now;
  }
}
#pragma endregion

void setup() {

  #pragma region POWER_SAVING
  WiFi.mode(WIFI_OFF); // Turn off WiFi to save power
  btStop();          // Turn off Bluetooth to save power
  esp_bt_controller_disable(); // Disable the BT controller
  // setCpuFrequencyMhz(80); // Set CPU frequency to 80 MHz to save power
  #pragma endregion
  
  #pragma region PIN_INITIALISATION
  pinMode(axisX.config.pinOutForeward, OUTPUT);
  pinMode(axisX.config.pinOutBackward, OUTPUT);
  pinMode(axisY.config.pinOutForeward, OUTPUT);
  pinMode(axisY.config.pinOutBackward, OUTPUT);
  
  pinMode(axisX.config.pinInputHome, INPUT);
  pinMode(axisX.config.pinInputRotationFeedback, INPUT);
  pinMode(axisY.config.pinInputHome, INPUT);
  pinMode(axisY.config.pinInputRotationFeedback, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(axisX.config.pinInputHome),
    engine1ValidHome,
    FALLING          // LOW → gedrückt (wegen Pullup)
  );
  attachInterrupt(
    digitalPinToInterrupt(axisX.config.pinInputRotationFeedback),
    engine1StepMade,
    FALLING          // HIGH → LOW
  );
  attachInterrupt(
    digitalPinToInterrupt(axisY.config.pinInputHome),
    engine2ValidHome,
    FALLING          // LOW → gedrückt (wegen Pullup)
  );
  attachInterrupt(
    digitalPinToInterrupt(axisY.config.pinInputRotationFeedback),
    engine2StepMade,
    FALLING          // HIGH → LOW
  );
  #pragma endregion
  
  Serial.begin(9600);

    if (DEBUG)
    {
      Serial.println("Homing Y axis...");
    }
    
  }
  
  
  const int LINES = 3;
void loop() {
  engineFindHome(&axisX);
  engineFindHome(&axisY);
    
  
  for (size_t i = 0; i < LINES; i++)
  {
    
    if(DEBUG) {
      Serial.println("Loop iteration: " + String(i));
    }
    engineStep(&axisY, 25, FOREWARD);
    engineStep(&axisX, 5, FOREWARD);
    engineFindHome(&axisY);
    engineStep(&axisX, 5, FOREWARD);
  }
  engineStep(&axisY, 25, FOREWARD);
  engineStep(&axisX, 5, FOREWARD);
  engineFindHome(&axisY);
  engineStep(&axisY, 2, FOREWARD);
}

