#pragma once
#include "ISoundEffect.h"

class Synth final : public ISoundEffect {
public:
    SoundEffectType type() const override { return SoundEffectType::Synth; }
    void update(float angularSpeed, float roll, float pitch) override;
    void render(float* output, int sampleCount, float sampleRate, float volume) override;
    float frequency() const;

private:
    volatile float toneFrequency = 250.0f;
    float phase = 0.0f;
};