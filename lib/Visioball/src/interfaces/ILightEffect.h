#pragma once

class Spatial;
struct LED;

class ILightEffect {
public:
  virtual ~ILightEffect() = default;
  virtual bool update(LED &led, const Spatial &spatial) const = 0;
};
