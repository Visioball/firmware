#pragma once

#include "interfaces/ISoundEffect.h"

class Beep : public ISoundEffect {
public:
  bool update(Speaker &speaker, const Spatial &spatial) const override;
};
