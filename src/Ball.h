#pragma once

#include "actuators/light/Light.h"
#include "interfaces/IActuator.h"
#include "motion/Motion.h"

class Ball {
private:
  Motion &motion_;
  Light &light_;
  IActuator &sound_;

public:
  Ball(Motion &motion, Light &light, IActuator &sound);

  bool begin();
  bool update();
  Motion &motion();
};
