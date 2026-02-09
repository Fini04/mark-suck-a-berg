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
    engineDirection(engine, BACKWARD);
    // If the switch is already pressed, the FALLING interrupt won't fire.
    if (digitalRead(engine->config.pinInputHome) == LOW) {
        engine->validHome = true;
    }
    if (!engine->validHome)
    {
        engineRunning(engine, true);
    } else {
        engineRunning(engine, false);
    }
}

void engineGoHome(Engine *engine) {
    engineDirection(engine, BACKWARD);
    if (engine->steps && engine->validHome)
    {
        engineRunning(engine, true);
    }
    engineRunning(engine, false);
}

volatile unsigned long lastStepCount = 0;
void engineStep(Engine *engine, bool direction, bool *success) {
    engineDirection(engine, direction);
    if (engine->steps > 0) {
        *success = false;
        engineRunning(engine, true);
        if (lastStepCount != engine->steps && DEBUG)
        {
            //logLine(String("Steps left: ") + engine->steps);
            lastStepCount = engine->steps;
        }
    } else {
        *success = true;
        engineRunning(engine, false);
    }
}
