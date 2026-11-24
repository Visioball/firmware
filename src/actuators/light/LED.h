#pragma once

#include <stddef.h>

#include "actuators/light/Color.h"
#include "motion/spatial/Vector3.h"

struct LED {
  size_t id = 0;
  Vector3 position;
  bool hasPosition = false;
  Color color;
};
