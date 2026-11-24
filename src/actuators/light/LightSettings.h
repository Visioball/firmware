#pragma once
#include <cstdint>

struct LightSettings {
    bool enabled = true;
    uint8_t intensity = 40;
    uint8_t red = 255;
    uint8_t green = 0;
    uint8_t blue = 0;
};
