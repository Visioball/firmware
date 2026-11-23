#include "Motion.h"

Motion::Motion(IMotionSensor &sensor, Orientation &orientation)
    : sensor_(sensor), orientation_(orientation) {}

bool Motion::begin() {
  orientation_.reset();
  return Motion::sensor_.begin();
}

bool Motion::update() {
  MotionSample sample;
  if (sensor_.read(sample)) { //returns true if read was successful and sample is set
    Motion::acceleration_ = sample.acceleration;
    Motion::gyroscope_ = sample.gyroscope;
    orientation_.update(sample);
    return true;
  }
  orientation_.reset();
  return false;
}

const Vector3 &Motion::getAccelerationValue() const { return Motion::acceleration_; }

const Vector3 &Motion::getGyroscopeValue() const { return Motion::gyroscope_; }
