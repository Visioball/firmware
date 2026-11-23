#pragma once

#include <array>
#include <stddef.h>

#include "interfaces/IActuator.h"
#include "interfaces/ILightEffect.h"
#include "interfaces/ILightOutput.h"
#include "LED.h"
#include "spatial/Spatial.h"

class Light : public IActuator {

  public:
  static constexpr size_t MAX_LEDS = 72;

  private:
  ILightOutput &output_;
  const Spatial &spatial_;
  ILightEffect *activeEffect_ = nullptr;
  std::array<LED, MAX_LEDS> leds_;
  size_t activeLedCount_;

public:
  Light(ILightOutput &output, const Spatial &spatial, size_t activeLedCount);
  void setEffect(ILightEffect &effect);
  bool setPosition(size_t index, const Vector3 &position);
  bool update() override;
};
