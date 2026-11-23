#pragma once
#include "ILightEffect.h"

class Solid final : public ILightEffect {
public:
    explicit Solid(Color color = {255, 0, 0}) : color(color) {}
    void setColor(Color value) { color = value; }
    Color update(const LED&, const Spatial&) const override { return color; }

private:
    Color color;
};
