#pragma once
#include "DspComponents.h"
#include "ISoundEffect.h"

class WavDSP final : public ISoundEffect {
public:
    SoundEffectType type() const override { return SoundEffectType::WavDSP; }
    bool initialize(const char* wavPath);
    void start() override;
    void update(float movement, float roll, float pitch) override;
    void render(float* output, int sampleCount, float sampleRate, float volume) override;

    float sampleRate() const;
    const char* storageType() const;
    const char* loadLog() const;

private:
    WavSource source;
    SampleVoice voice;
    DelayEffect delay;
    BitcrusherEffect bitcrusher;
    float pitchMultiplier = 1.0f;
};