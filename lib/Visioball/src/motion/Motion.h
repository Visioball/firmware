#pragma once

#include "common/Quaternion.h"
#include "common/Vector3.h"

class Motion {
public:
  // The IMU and orientation calculation have not been connected yet.
  bool begin();
  bool update();
  bool getOrientation(Quaternion &orientation) const;

private:
  Vector3 gyroscope_;
  Vector3 acceleration_;
  Quaternion orientation_;
  bool hasOrientation_ = false;
};
