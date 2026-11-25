#pragma once

#include "motion/spatial/Quaternion.h"
#include "interfaces/IMotionSensor.h"

class Orientation {
private:
  Quaternion orientation_;
  uint32_t lastSampleTime_ = 0;
  bool valid_ = false;

public:
  void reset();
  void update(const MotionSample &sample);
  bool getOrientation(Quaternion &orientation) const;
};
