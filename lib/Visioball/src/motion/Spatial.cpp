#include "Spatial.h"

Spatial::Spatial(const Motion &motion) : motion_(motion) {}

bool Spatial::getWorldPosition(const Vector3 &position,
                               Vector3 &worldPosition) const {
  Quaternion q;
  if (!motion_.getOrientation(q)) {
    return false;
  }

  // Rotate a local position by the unit orientation quaternion.
  const float tx = 2 * (q.y * position.z - q.z * position.y);
  const float ty = 2 * (q.z * position.x - q.x * position.z);
  const float tz = 2 * (q.x * position.y - q.y * position.x);
  worldPosition.x = position.x + q.w * tx + (q.y * tz - q.z * ty);
  worldPosition.y = position.y + q.w * ty + (q.z * tx - q.x * tz);
  worldPosition.z = position.z + q.w * tz + (q.x * ty - q.y * tx);
  return true;
}

bool Spatial::getUpValue(const Vector3 &position, float &value) const {
  Vector3 world;
  if (!getWorldPosition(position, world)) {
    return false;
  }
  value = world.z;
  return true;
}

bool Spatial::isUpperHalf(const Vector3 &position, bool &upper) const {
  float up;
  if (!getUpValue(position, up)) {
    return false;
  }
  upper = up >= 0;
  return true;
}
