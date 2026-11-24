#pragma once

#include "motion/Orientation.h"
#include "motion/spatial/Vector3.h"

class Spatial {
public:
  explicit Spatial(const Orientation &orientation);

  bool getUpValue(const Vector3 &position, float &value) const;
  bool getWorldPosition(const Vector3 &position, Vector3 &worldPosition) const;
  bool isUpperHalf(const Vector3 &position, bool &upper) const;

private:
  const Orientation &orientation_;
};
