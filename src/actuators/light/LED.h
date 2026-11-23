#pragma once

#include <stddef.h>

#include "actuators/light/Color.h"
#include "spatial/Vector3.h"

struct LED {
  size_t id = 0;
  Vector3 position;
  bool hasPosition = false;
  Color color;
};
