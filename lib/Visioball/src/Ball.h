#pragma once

#include "interfaces/IActuator.h"
#include "motion/Motion.h"

class Ball {
public:
  explicit Ball(Motion &motion);
  bool update(IActuator &actuator);
  Motion &motion();

private:
  Motion &motion_;
};
