#pragma once
#include <cmath>
#include "Vector3.h"

struct Quaternion {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    static Quaternion fromTilt(float rollDegrees, float pitchDegrees) {
        constexpr float HALF_RADIANS = 0.00872664626f;
        const float sr = std::sin(rollDegrees * HALF_RADIANS);
        const float cr = std::cos(rollDegrees * HALF_RADIANS);
        const float sp = std::sin(pitchDegrees * HALF_RADIANS);
        const float cp = std::cos(pitchDegrees * HALF_RADIANS);
        return {cr * cp, sr * cp, cr * sp, -sr * sp};
    }

    Vector3 rotate(const Vector3& v) const {
        const Vector3 t{2.0f * (y * v.z - z * v.y),
                        2.0f * (z * v.x - x * v.z),
                        2.0f * (x * v.y - y * v.x)};
        return {v.x + w * t.x + y * t.z - z * t.y,
                v.y + w * t.y + z * t.x - x * t.z,
                v.z + w * t.z + x * t.y - y * t.x};
    }
};
