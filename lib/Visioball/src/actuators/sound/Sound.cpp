#include "Sound.h"

Sound::Sound(const Spatial &spatial) : spatial_(spatial) {
  for (size_t i = 0; i < speakers_.size(); ++i) {
    speakers_[i].id = i;
  }
}

void Sound::setEffect(ISoundEffect &effect) { activeEffect_ = &effect; }

bool Sound::setPosition(size_t index, const Vector3 &position) {
  if (index >= speakers_.size()) {
    return false;
  }
  speakers_[index].position = position;
  speakers_[index].hasPosition = true;
  return true;
}

bool Sound::update() {
  if (!enabled_) {
    return true;
  }
  if (activeEffect_ == nullptr) {
    return false;
  }
  for (Speaker &speaker : speakers_) {
    if (!activeEffect_->update(speaker, spatial_)) {
      return false;
    }
  }
  return true;
}
