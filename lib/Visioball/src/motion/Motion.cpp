#include "Motion.h"

bool Motion::begin() { return false; }

bool Motion::update() { return false; }

bool Motion::getOrientation(Quaternion &orientation) const {
  if (!hasOrientation_) {
    return false;
  }
  orientation = orientation_;
  return true;
}
