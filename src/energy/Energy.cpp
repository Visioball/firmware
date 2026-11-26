#include "Energy.h"

Energy::Energy(BatteryLevelMeter &batteryLevelMeter)
    : batteryLevelMeter_(&batteryLevelMeter) {}

bool Energy::getBatteryLevel(uint8_t &percent) {
  uint8_t measuredPercent = 0;
  if (!batteryLevelMeter_ || !batteryLevelMeter_->readLevel(measuredPercent) ||
      measuredPercent > 100) {
    return false;
  }
  percent = measuredPercent;
  return true;
}

bool Energy::updateBleConnection(bool connected) {
  if (bleConnected_ == connected) return false;
  bleConnected_ = connected;
  return true;
}

bool Energy::lightAllowed() const {
  return bleConnected_;
}

void Energy::setPowerSavingEnabled(bool enabled, uint32_t now) {
  powerSavingEnabled_ = enabled;
  recordActivity(now);
}

void Energy::recordActivity(uint32_t now) {
  lastActivityMs_ = now;
}

bool Energy::shouldSleep(uint32_t now) const {
  return powerSavingEnabled_ && !bleConnected_ &&
         static_cast<uint32_t>(now - lastActivityMs_) >= INACTIVITY_MS;
}

