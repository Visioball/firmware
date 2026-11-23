#pragma once

#include <Adafruit_NeoPixel.h>

#include "interfaces/ILightOutput.h"

class NeoPixelOutput : public ILightOutput {
public:
  explicit NeoPixelOutput(Adafruit_NeoPixel &strip);
  void clear() override;
  void setBrightness(uint8_t brightness) override;
  void setPixel(size_t index, const Color &color) override;
  void show() override;

private:
  Adafruit_NeoPixel &strip_;
};
