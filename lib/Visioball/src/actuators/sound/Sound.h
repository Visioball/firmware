#pragma once

#include <array>

#include "interfaces/IActuator.h"
#include "interfaces/ISoundEffect.h"
#include "motion/Spatial.h"
#include "Speaker.h"

class Sound : public IActuator {
public:
  static constexpr size_t SPEAKER_COUNT = 6;

  explicit Sound(const Spatial &spatial);
  void setEffect(ISoundEffect &effect);
  bool setPosition(size_t index, const Vector3 &position);
  bool update() override;

private:
  const Spatial &spatial_;
  ISoundEffect *activeEffect_ = nullptr;
  std::array<Speaker, SPEAKER_COUNT> speakers_;
};
