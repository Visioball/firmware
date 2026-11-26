#pragma once

#include <stdint.h>

class BatteryLevelMeter {
public:
  virtual ~BatteryLevelMeter() = default;
  virtual bool readLevel(uint8_t &percent) = 0;
};
