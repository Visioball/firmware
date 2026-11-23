#pragma once
#include "LED.h"

class Spatial;

class ILightEffect {
public:
    virtual ~ILightEffect() = default;
    virtual Color update(const LED& led, const Spatial& spatial) const = 0;
    virtual bool requiresPositions() const { return false; }
};
