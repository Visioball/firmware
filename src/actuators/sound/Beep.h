#pragma once
#include "interfaces/ISoundEffect.h"
#include <cmath>

class Beep final : public ISoundEffect {
public:
    SoundEffectType type() const override { return SoundEffectType::Beep; }
    void update(float, float, float) override {}
    void start() override { remainingSeconds = 0.12f; }
    void triggerHeartbeat(float) override { start(); }
    void render(float* output, int count, float sampleRate, float volume) override {
        constexpr float FULL_CYCLE = 6.28318530718f;
        for (int i = 0; i < count && remainingSeconds > 0.0f; ++i) {
            output[i] = std::sin(phase * FULL_CYCLE) * volume * 0.15f;
            phase += 440.0f / sampleRate;
            phase -= std::floor(phase);
            remainingSeconds -= 1.0f / sampleRate;
        }
    }

private:
    float phase = 0.0f;
    float remainingSeconds = 0.0f;
};
