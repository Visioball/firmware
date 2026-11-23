#include "Solid.h"

#include "actuators/light/LED.h"

Solid::Solid(const Color &color) : color_(color) {}

void Solid::setColor(const Color &color) { color_ = color; }

bool Solid::update(LED &led, const Spatial &) const {
  led.color = color_;
  return true;
}
