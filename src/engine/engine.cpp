const int DEBUG = 1;

#include "engine/engine.hpp"

void engineRunning(Engine *engine, bool running) {
    if (!running) {
        digitalWrite(engine->config.pinOutForeward, LOW);
        digitalWrite(engine->config.pinOutBackward, LOW);
        return;
    }
    if (engine->moveDirection == FOREWARD)
    {
        digitalWrite(engine->config.pinOutForeward, HIGH);
        digitalWrite(engine->config.pinOutBackward, LOW);
    } else {
        digitalWrite(engine->config.pinOutForeward, LOW);
        digitalWrite(engine->config.pinOutBackward, HIGH);
    }
}

void engineDirection(Engine *engine, bool direction) {
    engine->moveDirection = direction;
}

void engineFindHome(Engine *engine) {
    engine->validHome = false;
    
    if (DEBUG) {
        Serial.println("Finding home...");
    }
    engineDirection(engine, BACKWARD);
    while (!engine->validHome)
    {
        engineRunning(engine, true);
    }
    if (DEBUG)
    {
        Serial.println("Home found!");
    }
    engineRunning(engine, false);
    
}

void engineGoHome(Engine *engine) {
    engineDirection(engine, BACKWARD);
    while (engine->steps)
    {
        engineRunning(engine, true);
    }
    engineRunning(engine, false);
}

volatile unsigned long lastStepCount = 0;
void engineStep(Engine *engine, unsigned long steps, bool direction) {
    engineDirection(engine, direction);
    engine->steps = steps;
    while (engine->steps)
    {
        engineRunning(engine, true);
        if (lastStepCount != engine->steps)
        {
            Serial.print("Steps left: ");
            Serial.println(engine->steps);
            lastStepCount = engine->steps;
        }
    }
    engineRunning(engine, false);
}