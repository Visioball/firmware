#include "Light.h"

Light::Light(ILightOutput &output, const Spatial &spatial, size_t activeLedCount)
    : output_(output), spatial_(spatial), activeLedCount_(activeLedCount) {
  for (size_t i = 0; i < leds_.size(); ++i) {
    leds_[i].id = i;
  }
}

void Light::setEffect(ILightEffect &effect) { activeEffect_ = &effect; }

bool Light::setPosition(size_t index, const Vector3 &position) {
  if (index >= leds_.size()) {
    return false;
  }
  leds_[index].position = position;
  leds_[index].hasPosition = true;
  return true;
}

bool Light::update() {
  if (activeLedCount_ > leds_.size()) {
    return false;
  }
  if (!enabled_) {
    output_.clear();
    output_.show();
    return true;
  }
  if (activeEffect_ == nullptr) {
    return false;
  }
  for (size_t i = 0; i < activeLedCount_; ++i) {
    if (!activeEffect_->update(leds_[i], spatial_)) {
      return false;
    }
  }
  output_.setBrightness(level_);
  output_.clear();
  for (size_t i = 0; i < activeLedCount_; ++i) {
    output_.setPixel(i, leds_[i].color);
  }
  output_.show();
  return true;
}
