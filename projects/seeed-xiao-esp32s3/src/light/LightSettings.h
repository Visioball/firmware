#pragma once
#include <cstdint>

struct LightSettings {
    bool enabled = false;
    uint8_t intensity = 255;
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};
