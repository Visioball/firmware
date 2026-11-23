#include "HorizontalSplit.h"

#include "actuators/light/LED.h"
#include "spatial/Spatial.h"

HorizontalSplit::HorizontalSplit(const Color &topColor,
                                 const Color &bottomColor)
    : topColor_(topColor), bottomColor_(bottomColor) {}

bool HorizontalSplit::update(LED &led, const Spatial &spatial) const {
  bool upper;
  if (!led.hasPosition || !spatial.isUpperHalf(led.position, upper)) {
    return false;
  }
  led.color = upper ? topColor_ : bottomColor_;
  return true;
}
