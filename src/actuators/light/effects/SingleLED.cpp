#include "SingleLED.h"

#include "actuators/light/LED.h"
#include "spatial/Spatial.h"

SingleLED::SingleLED(const Vector3 *positions, size_t count, const Color &color)
    : positions_(positions), count_(count), color_(color) {}

bool SingleLED::update(LED &led, const Spatial &spatial) const {
  if (positions_ == nullptr || count_ == 0 || !led.hasPosition ||
      led.id >= count_) {
    return false;
  }

  float highest;
  if (!spatial.getUpValue(positions_[0], highest)) {
    return false;
  }
  size_t highestIndex = 0;
  for (size_t i = 1; i < count_; ++i) {
    float up;
    if (!spatial.getUpValue(positions_[i], up)) {
      return false;
    }
    if (up > highest) {
      highest = up;
      highestIndex = i;
    }
  }
  led.color = led.id == highestIndex ? color_ : Color{};
  return true;
}
