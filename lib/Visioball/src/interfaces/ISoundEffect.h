#pragma once

class Spatial;
struct Speaker;

class ISoundEffect {
public:
  virtual ~ISoundEffect() = default;
  virtual bool update(Speaker &speaker, const Spatial &spatial) const = 0;
};
