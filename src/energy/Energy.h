#pragma once

#include "BatteryLevelMeter.h"

class Energy {
public:
  static constexpr uint32_t INACTIVITY_MS = 5 * 60 * 1000;

  Energy() = default;
  explicit Energy(BatteryLevelMeter &batteryLevelMeter);

  bool getBatteryLevel(uint8_t &percent);
  bool updateBleConnection(bool connected);
  bool lightAllowed() const;
  void setPowerSavingEnabled(bool enabled, uint32_t now);
  void recordActivity(uint32_t now);
  bool shouldSleep(uint32_t now) const;

private:
  BatteryLevelMeter *batteryLevelMeter_ = nullptr;
  bool bleConnected_ = false;
  bool powerSavingEnabled_ = false;
  uint32_t lastActivityMs_ = 0;
};
