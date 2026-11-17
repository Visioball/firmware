#pragma once

#include "Motion.h"
#include "common/Vector3.h"

class Spatial {
public:
  explicit Spatial(const Motion &motion);

  bool getUpValue(const Vector3 &position, float &value) const;
  bool getWorldPosition(const Vector3 &position, Vector3 &worldPosition) const;
  bool isUpperHalf(const Vector3 &position, bool &upper) const;

private:
  const Motion &motion_;
};
