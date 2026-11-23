#include "MPU9250Sensor.h"

MPU9250Sensor::MPU9250Sensor(TwoWire &wire, uint8_t address)
    : imu_(&wire, address) {}

bool MPU9250Sensor::begin() { return imu_.init(); }

bool MPU9250Sensor::read(MotionSample &sample) {
  const xyzFloat acceleration = imu_.getGValues();
  const xyzFloat gyroscope = imu_.getGyrValues();

  sample.acceleration.x = acceleration.x;
  sample.acceleration.y = acceleration.y;
  sample.acceleration.z = acceleration.z;
  sample.gyroscope.x = gyroscope.x;
  sample.gyroscope.y = gyroscope.y;
  sample.gyroscope.z = gyroscope.z;
  sample.timestampMs = millis();
  return true;
  //TODO: Implement error handling for failed sensor reads. Now always returns true.
}
