#pragma once
#include <array>
#include <cstdint>
#include "../../spatial/Vector3.h"

enum class SpeakerId : uint8_t { XPositive, XNegative, YPositive, YNegative, ZPositive, ZNegative };

struct Speaker {
    SpeakerId id;
    Vector3 position;
    float gain = 1.0f;
};

using Speakers = std::array<Speaker, 6>;

inline Speakers axisSpeakers() {
    return {{{SpeakerId::XPositive, {1, 0, 0}},
             {SpeakerId::XNegative, {-1, 0, 0}},
             {SpeakerId::YPositive, {0, 1, 0}},
             {SpeakerId::YNegative, {0, -1, 0}},
             {SpeakerId::ZPositive, {0, 0, 1}},
             {SpeakerId::ZNegative, {0, 0, -1}}}};
}
