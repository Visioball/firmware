#pragma once

#include <stddef.h>
#include <stdint.h>

#include "common/Color.h"

class ILightOutput {
public:
  virtual ~ILightOutput() = default;
  virtual void clear() = 0;
  virtual void setBrightness(uint8_t brightness) = 0;
  virtual void setPixel(size_t index, const Color &color) = 0;
  virtual void show() = 0;
};
