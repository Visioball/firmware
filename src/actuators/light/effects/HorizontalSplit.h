#pragma once

#include "actuators/light/Color.h"
#include "interfaces/ILightEffect.h"

class HorizontalSplit : public ILightEffect {
public:
  HorizontalSplit(const Color &topColor, const Color &bottomColor);
  void setTopColor(const Color &color) { topColor_ = color; }
  bool update(LED &led, const Spatial &spatial) const override;

private:
  Color topColor_;
  Color bottomColor_;
};
