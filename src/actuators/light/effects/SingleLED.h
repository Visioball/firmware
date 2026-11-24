#pragma once

#include <stddef.h>

#include "actuators/light/Color.h"
#include "motion/spatial/Vector3.h"
#include "interfaces/ILightEffect.h"

class SingleLED : public ILightEffect {
public:
  SingleLED(const Vector3 *positions, size_t count, const Color &color);
  void setColor(const Color &color) { color_ = color; }
  bool update(LED &led, const Spatial &spatial) const override;

private:
  const Vector3 *positions_;
  size_t count_;
  Color color_;
};
