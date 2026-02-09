#ifndef AXIS_CONFIGURATION
#define AXIS_CONFIGURATION

#include "./engine/engine.hpp"

/**
 * Configuration for the X axis of the Gantry.
 */
Engine axisX = {
  .config = {
    .pinOutForeward = 33,
    .pinOutBackward = 32,
    .pinInputHome = 36, //Input only pin
    .pinInputRotationFeedback = 39, //Input only pin
    .stepsPerRotation = 3,
    .maxStepps = 1000
  },
  .steps = 0,
  .moveDirection = FOREWARD,
  .validHome = false
};


/**
 * Configuration for the Y axis of the Gantry.
 */
Engine axisY = {
  .config = {
    .pinOutForeward = 27,
    .pinOutBackward = 26,
    .pinInputHome = 34, //Input only pin
    .pinInputRotationFeedback = 35, //Input only pin
    .stepsPerRotation = 3,
    .maxStepps = 1000
  },
  .steps = 0,
  .moveDirection = FOREWARD,
  .validHome = false
};

#endif