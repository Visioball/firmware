#pragma once

#include <stddef.h>

#include "common/Vector3.h"

struct Speaker {
  size_t id = 0;
  Vector3 position;
  bool hasPosition = false;
  float gain = 1;
};
