#pragma once

#include "common/Color.h"
#include "interfaces/ILightEffect.h"

class Solid : public ILightEffect {
public:
  explicit Solid(const Color &color);
  void setColor(const Color &color);
  bool update(LED &led, const Spatial &spatial) const override;

private:
  Color color_;
};
