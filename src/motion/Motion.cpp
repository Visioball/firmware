#include "Motion.h"

#include <cmath>

namespace {
constexpr float DEGREES_TO_RADIANS = 0.01745329252f;
constexpr float ROLLING_SPEED_SMOOTHING = 0.3f;
}

Motion::Motion(IMotionSensor &sensor, Orientation &orientation)
    : sensor_(sensor), orientation_(orientation) {}

bool Motion::begin() {
  orientation_.reset();
  rollingSpeedValid_ = false;
  return Motion::sensor_.begin();
}

bool Motion::update() {
  MotionSample sample;
  if (sensor_.read(sample)) { //returns true if read was successful and sample is set
    Motion::acceleration_ = sample.acceleration;
    Motion::gyroscope_ = sample.gyroscope;
    orientation_.update(sample);
    Quaternion q;
    if (orientation_.getOrientation(q)) {
      const float upX = 2.0f * (q.x * q.z - q.w * q.y);
      const float upY = 2.0f * (q.y * q.z + q.w * q.x);
      const float upZ = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
      const Vector3 &gyro = sample.gyroscope;
      const float x = gyro.y * upZ - gyro.z * upY;
      const float y = gyro.z * upX - gyro.x * upZ;
      const float z = gyro.x * upY - gyro.y * upX;
      const float speed = std::hypot(std::hypot(x, y), z) * DEGREES_TO_RADIANS;
      if (std::isfinite(speed)) {
        rollingSpeed_ = rollingSpeedValid_
                            ? rollingSpeed_ + ROLLING_SPEED_SMOOTHING * (speed - rollingSpeed_)
                            : speed;
        rollingSpeedValid_ = true;
      } else {
        rollingSpeedValid_ = false;
      }
    } else {
      rollingSpeedValid_ = false;
    }
    return true;
  }
  orientation_.reset();
  rollingSpeedValid_ = false;
  return false;
}

const Vector3 &Motion::getAccelerationValue() const { return Motion::acceleration_; }

const Vector3 &Motion::getGyroscopeValue() const { return Motion::gyroscope_; }

bool Motion::getRollingSpeed(float &radiansPerSecond) const {
  if (!rollingSpeedValid_) return false;
  radiansPerSecond = rollingSpeed_;
  return true;
}
