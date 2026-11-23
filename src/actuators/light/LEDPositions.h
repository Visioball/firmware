#pragma once

#include <stddef.h>

#include "spatial/Vector3.h"

constexpr size_t LED_POSITION_COUNT = 18;

// Sensor frame: +Y towards USB, +Z towards the sensor face, +X to its right.
// Each photographed face is projected onto a ring on its half of the ball.
const Vector3 LED_POSITIONS[LED_POSITION_COUNT] = {
    {0.45452f, -0.54168f, -0.70711f},  // 1, lower face
    {0.00000f, -0.70711f, -0.70711f},  // 2
    {-0.45452f, -0.54168f, -0.70711f}, // 3
    {-0.69636f, -0.12279f, -0.70711f}, // 4
    {-0.61237f, 0.35355f, -0.70711f},  // 5
    {-0.24184f, 0.66446f, -0.70711f},  // 6
    {0.24184f, 0.66446f, -0.70711f},   // 7
    {0.61237f, 0.35355f, -0.70711f},   // 8
    {0.69636f, -0.12279f, -0.70711f},  // 9
    {-0.45452f, -0.54168f, 0.70711f},  // 10, sensor face
    {0.00000f, -0.70711f, 0.70711f},   // 11
    {0.45452f, -0.54168f, 0.70711f},   // 12
    {0.69636f, -0.12279f, 0.70711f},   // 13
    {0.61237f, 0.35355f, 0.70711f},    // 14
    {0.24184f, 0.66446f, 0.70711f},    // 15
    {-0.24184f, 0.66446f, 0.70711f},   // 16
    {-0.61237f, 0.35355f, 0.70711f},   // 17
    {-0.69636f, -0.12279f, 0.70711f},  // 18
};
