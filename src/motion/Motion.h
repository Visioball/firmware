#pragma once

#include "interfaces/IMotionSensor.h"
#include "motion/Orientation.h"

class Motion {
private:
  IMotionSensor &sensor_;
  Vector3 gyroscope_;
  Vector3 acceleration_;
  Orientation &orientation_;
  float rollingSpeed_ = 0.0f;
  bool rollingSpeedValid_ = false;

public:
  Motion(IMotionSensor &sensor, Orientation &orientation);

  bool begin();
  bool update();

  const Vector3 &getAccelerationValue() const;
  const Vector3 &getGyroscopeValue() const;
  bool getRollingSpeed(float &radiansPerSecond) const;
};
