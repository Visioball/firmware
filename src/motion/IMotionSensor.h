#pragma once

#include <stdint.h>

#include "spatial/Vector3.h"

struct MotionSample {
  // Sensor-local acceleration in g and angular velocity in degrees per second.
  Vector3 acceleration;
  Vector3 gyroscope;
  // Monotonic milliseconds; unsigned subtraction handles clock rollover.
  uint32_t timestampMs = 0;
};

class IMotionSensor {
public:
  virtual ~IMotionSensor() = default;
  virtual bool begin() = 0;
  virtual bool read(MotionSample &sample) = 0;
};
