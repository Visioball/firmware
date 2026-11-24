#pragma once
#include "actuators/sound/SoundEffectType.h"

class ISoundEffect {
public:
    virtual ~ISoundEffect() = default;
    virtual SoundEffectType type() const = 0;
    virtual void update(float movement, float roll, float pitch) = 0;
    virtual void render(float* output, int count, float sampleRate, float volume) = 0;
    virtual void start() {}
    virtual void triggerHeartbeat(float intensity) {}
};
