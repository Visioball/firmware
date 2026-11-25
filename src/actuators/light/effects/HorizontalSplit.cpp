#include "HorizontalSplit.h"

#include "actuators/light/LED.h"
#include "motion/spatial/Spatial.h"

namespace {
constexpr float FADE_WIDTH = 0.15f;

uint8_t blendChannel(uint8_t bottom, uint8_t top, float mix) {
  return static_cast<uint8_t>(bottom + (top - bottom) * mix + 0.5f);
}
}

HorizontalSplit::HorizontalSplit(const Color &topColor,
                                 const Color &bottomColor)
    : topColor_(topColor), bottomColor_(bottomColor) {}

bool HorizontalSplit::update(LED &led, const Spatial &spatial) const {
  float up;
  if (!led.hasPosition || !spatial.getUpValue(led.position, up)) {
    return false;
  }
  if (up <= 0.0f) {
    led.color = bottomColor_;
  } else if (up >= FADE_WIDTH) {
    led.color = topColor_;
  } else {
    const float t = up / FADE_WIDTH;
    const float mix = t * t * (3.0f - 2.0f * t);
    led.color = {
        blendChannel(bottomColor_.red, topColor_.red, mix),
        blendChannel(bottomColor_.green, topColor_.green, mix),
        blendChannel(bottomColor_.blue, topColor_.blue, mix),
        blendChannel(bottomColor_.white, topColor_.white, mix),
    };
  }
  return true;
}
