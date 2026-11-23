#include "LavaGlow.h"

#include <math.h>

#include "actuators/light/LED.h"

namespace {

float clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

uint8_t channel(float value) {
  return static_cast<uint8_t>(clamp(value, 0.0f, 255.0f));
}

Color colorFromHue(float hue, float brightness) {
  const float sector = hue / 60.0f;
  const float intermediate =
      brightness * (1.0f - fabsf(fmodf(sector, 2.0f) - 1.0f));
  float red = 0.0f;
  float green = 0.0f;

  if (sector < 1.0f) {
    red = brightness;
    green = intermediate;
  } else {
    red = intermediate;
    green = brightness;
  }

  return Color{channel(red), channel(green), 0, 0};
}

} // namespace

void LavaGlow::setControls(float xControlDegrees, float yControlDegrees,
                           float zControlDegrees, uint32_t timeMs) {
  xControlDegrees_ = clamp(xControlDegrees, -90.0f, 90.0f);
  yControlDegrees_ = clamp(yControlDegrees, -90.0f, 90.0f);
  zControlDegrees_ = clamp(zControlDegrees, -90.0f, 90.0f);
  timeMs_ = timeMs;
}

bool LavaGlow::update(LED &led, const Spatial &) const {
  constexpr float PI = 3.14159265358979323846f;
  constexpr float MIN_ANIMATION_HZ = 0.25f;
  constexpr float MAX_ANIMATION_HZ = 2.0f;

  const float xPosition = (xControlDegrees_ + 90.0f) / 180.0f;
  const float yPosition = (yControlDegrees_ + 90.0f) / 180.0f;
  const float baseBrightness = 40.0f + xPosition * 215.0f;
  const float hue = yPosition * 60.0f;
  const float zPosition = (zControlDegrees_ + 90.0f) / 180.0f;
  const float animationHz =
      MIN_ANIMATION_HZ + zPosition * (MAX_ANIMATION_HZ - MIN_ANIMATION_HZ);
  const float timeSeconds = static_cast<float>(timeMs_) / 1000.0f;
  const float phase = timeSeconds * animationHz * 2.0f * PI;
  const float ledPhase = static_cast<float>(led.id);
  const float wave =
      0.5f + 0.32f * sinf(phase + ledPhase * 0.42f) +
      0.18f * sinf(phase * 0.47f + ledPhase * 1.17f);
  const float glow = 0.55f + 0.45f * clamp(wave, 0.0f, 1.0f);

  led.color = colorFromHue(hue, baseBrightness * glow);
  return true;
}
