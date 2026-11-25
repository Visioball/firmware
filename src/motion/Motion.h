#pragma once

#include "interfaces/IMotionSensor.h"
#include "motion/Orientation.h"

class Motion {
private:
  IMotionSensor &sensor_;
  Vector3 gyroscope_;
  Vector3 acceleration_;
  Orientation &orientation_;

public:
  Motion(IMotionSensor &sensor, Orientation &orientation);

  bool begin();
  bool update();

  const Vector3 &getAccelerationValue() const;
  const Vector3 &getGyroscopeValue() const;
};
