#pragma once

#include <Adafruit_NeoPixel.h>

#include "interfaces/ILightOutput.h"

class LEDOutput : public ILightOutput {
public:
  explicit LEDOutput(Adafruit_NeoPixel &strip);
  LEDOutput(Adafruit_NeoPixel &firstStrip, Adafruit_NeoPixel &secondStrip,
            size_t secondStripStartIndex);
  void clear() override;
  void setBrightness(uint8_t brightness) override;
  void setPixel(size_t index, const Color &color) override;
  void show() override;

private:
  Adafruit_NeoPixel &strip_;
  Adafruit_NeoPixel *secondStrip_ = nullptr;
  size_t secondStripStartIndex_ = 0;
};
