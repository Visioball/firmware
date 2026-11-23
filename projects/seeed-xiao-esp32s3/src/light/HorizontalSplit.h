#pragma once
#include "ILightEffect.h"
#include "../spatial/Spatial.h"

class HorizontalSplit final : public ILightEffect {
public:
    HorizontalSplit(Color top, Color bottom) : topColor(top), bottomColor(bottom) {}
    Color update(const LED& led, const Spatial& spatial) const override {
        return spatial.isUpperHalf(led.position) ? topColor : bottomColor;
    }
    bool requiresPositions() const override { return true; }

private:
    Color topColor;
    Color bottomColor;
};
