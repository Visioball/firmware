#pragma once
#include "Color.h"
#include "../spatial/Vector3.h"

struct LED {
    uint16_t id = 0;
    Vector3 position;
    Color color;
    bool positioned = false;
};
