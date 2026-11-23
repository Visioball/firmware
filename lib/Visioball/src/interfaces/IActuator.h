#pragma once

#include <stdint.h>

class IActuator {
public:
  virtual ~IActuator() = default;
  virtual bool update() = 0;

  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }
  void setLevel(uint8_t level) { level_ = level; }
  uint8_t getLevel() const { return level_; }

protected:
  bool enabled_ = false;
  uint8_t level_ = 0;
};
