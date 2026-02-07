#ifndef ENGINE_H
#define ENGINE_H

#include <Arduino.h>
#include <esp_types.h>

#define FOREWARD 1
#define BACKWARD 0

typedef struct {
  uint8_t pinOutForeward;
  uint8_t pinOutBackward;
  uint8_t pinInputHome;
  uint8_t pinInputEndstop;
  uint8_t pinInputRotationFeedback;
  uint16_t stepsPerRotation;
  uint16_t maxStepps;
} EngineConfiguration;

typedef struct {
  EngineConfiguration config;
  uint16_t steps;
  bool moveDirection;
  bool validHome;
} Engine;

void engineRunning(Engine *engine, bool running);
void engineDirection(Engine *engine, bool direction);
void engineGoHome(Engine *engine); //not recomendet to use because of inaccuracy of the physical build. Use engineFindHome() instat
void engineFindHome(Engine *engine);
void engineStep(Engine *engine, unsigned long steps, bool direction);

#endif