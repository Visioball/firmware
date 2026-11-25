#pragma once

#include <MPU9250_WE.h>
#include <Wire.h>

#include "interfaces/IMotionSensor.h"

class MPU9250Sensor : public IMotionSensor {
public:
  MPU9250Sensor(TwoWire &wire, uint8_t address);

  bool begin() override;
  bool read(MotionSample &sample) override;

private:
  MPU9250_WE imu_;
};
