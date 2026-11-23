#include "Orientation.h"

#include <math.h>

namespace {

constexpr float DEGREES_TO_RADIANS = 0.01745329252f;
constexpr float ACCEL_CORRECTION = 2.0f;
constexpr uint32_t MAX_SAMPLE_GAP_MS = 250;

} // namespace

void Orientation::reset() {
  orientation_ = Quaternion{};
  lastSampleTime_ = 0;
  valid_ = false;
}

void Orientation::update(const MotionSample &sample) {
  const Vector3 &accel = sample.acceleration;
  const Vector3 &gyro = sample.gyroscope;
  if (!isfinite(gyro.x) || !isfinite(gyro.y) || !isfinite(gyro.z)) {
    reset();
    return;
  }

  const float magnitude = sqrtf(accel.x * accel.x + accel.y * accel.y +
                                accel.z * accel.z);
  const bool gravityAvailable = isfinite(magnitude) &&
                                magnitude >= 0.8f && magnitude <= 1.2f;
  const uint32_t elapsedMs = sample.timestampMs - lastSampleTime_;
  if (valid_ && elapsedMs > MAX_SAMPLE_GAP_MS) {
    reset();
  }

  if (!valid_) {
    if (!gravityAvailable) {
      return;
    }

    const float x = accel.x / magnitude;
    const float y = accel.y / magnitude;
    const float z = accel.z / magnitude;
    const float horizontal = sqrtf(x * x + y * y);
    if (horizontal < 0.000001f) {
      orientation_.w = z >= 0.0f ? 1.0f : 0.0f;
      orientation_.x = z >= 0.0f ? 0.0f : 1.0f;
    } else {
      const float halfAngle = atan2f(horizontal, z) * 0.5f;
      orientation_.w = cosf(halfAngle);
      orientation_.x = y * sinf(halfAngle) / horizontal;
      orientation_.y = -x * sinf(halfAngle) / horizontal;
    }
    orientation_.z = 0.0f;
    lastSampleTime_ = sample.timestampMs;
    valid_ = true;
    return;
  }

  const float dt = static_cast<float>(elapsedMs) / 1000.0f;
  lastSampleTime_ = sample.timestampMs;
  float wx = gyro.x * DEGREES_TO_RADIANS;
  float wy = gyro.y * DEGREES_TO_RADIANS;
  float wz = gyro.z * DEGREES_TO_RADIANS;

  if (gravityAvailable) {
    const float ax = accel.x / magnitude;
    const float ay = accel.y / magnitude;
    const float az = accel.z / magnitude;
    const Quaternion &q = orientation_;
    // Compare measured up with world up rotated into the sensor's frame.
    const float upX = 2.0f * (q.x * q.z - q.w * q.y);
    const float upY = 2.0f * (q.y * q.z + q.w * q.x);
    const float upZ = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    wx += ACCEL_CORRECTION * (ay * upZ - az * upY);
    wy += ACCEL_CORRECTION * (az * upX - ax * upZ);
    wz += ACCEL_CORRECTION * (ax * upY - ay * upX);
  }

  const float speed = sqrtf(wx * wx + wy * wy + wz * wz);
  if (!isfinite(speed)) {
    reset();
    return;
  }
  if (speed == 0.0f || dt == 0.0f) {
    return;
  }

  const float halfAngle = speed * dt * 0.5f;
  const float scale = sinf(halfAngle) / speed;
  const float dw = cosf(halfAngle);
  const float dx = wx * scale;
  const float dy = wy * scale;
  const float dz = wz * scale;
  const Quaternion previous = orientation_;
  orientation_.w = previous.w * dw - previous.x * dx - previous.y * dy - previous.z * dz;
  orientation_.x = previous.w * dx + previous.x * dw + previous.y * dz - previous.z * dy;
  orientation_.y = previous.w * dy - previous.x * dz + previous.y * dw + previous.z * dx;
  orientation_.z = previous.w * dz + previous.x * dy - previous.y * dx + previous.z * dw;

  const float norm = sqrtf(orientation_.w * orientation_.w +
                           orientation_.x * orientation_.x +
                           orientation_.y * orientation_.y +
                           orientation_.z * orientation_.z);
  if (!isfinite(norm) || norm == 0.0f) {
    reset();
    return;
  }
  orientation_.w /= norm;
  orientation_.x /= norm;
  orientation_.y /= norm;
  orientation_.z /= norm;
}

bool Orientation::getOrientation(Quaternion &orientation) const {
  if (!valid_) {
    return false;
  }
  orientation = orientation_;
  return true;
}
