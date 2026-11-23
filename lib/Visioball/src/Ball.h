#pragma once

#include "actuators/light/Light.h"
#include "actuators/sound/Sound.h"
#include "motion/Motion.h"

class Ball {
private:
  Motion &motion_;
  Light &light_;
  Sound &sound_;

public:
  Ball(Motion &motion, Light &light, Sound &sound);

  bool begin();
  bool update();
  Motion &motion();
};
