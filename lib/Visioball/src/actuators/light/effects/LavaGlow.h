#pragma once

#include <stdint.h>

#include "interfaces/ILightEffect.h"

class LavaGlow : public ILightEffect {
public:
  void setControls(float xControlDegrees, float yControlDegrees,
                   float zControlDegrees, uint32_t timeMs);
  bool update(LED &led, const Spatial &spatial) const override;

private:
  float xControlDegrees_ = 0.0f;
  float yControlDegrees_ = 0.0f;
  float zControlDegrees_ = -90.0f;
  uint32_t timeMs_ = 0;
};
