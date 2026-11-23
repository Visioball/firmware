#pragma once

#include "interfaces/ISoundEffect.h"

class Spotify : public ISoundEffect {
public:
  bool update(Speaker &speaker, const Spatial &spatial) const override;
};
