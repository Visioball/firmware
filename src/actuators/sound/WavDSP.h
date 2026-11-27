#pragma once
#include "DspComponents.h"
#include "interfaces/ISoundEffect.h"

class WavDSP final : public ISoundEffect {
public:
    SoundEffectType type() const override { return SoundEffectType::WavDSP; }
    static constexpr int MAX_TRACKS = 8;
    int scanFiles();
    int count() const { return trackCount; }
    const char* name(int index) const;
    const char* path(int index) const;
    bool initialize(const char* wavPath, float outputRate = 0.0f);
    void unload() { source.freeBuffer(); }
    void start() override;
    void update(float, float, float) override {}
    void setRollingSpeed(float radiansPerSecond);
    void render(float* output, int sampleCount, float sampleRate, float volume) override;

    float sampleRate() const;
    const char* storageType() const;
    const char* loadLog() const;

private:
    WavSource source;
    SampleVoice voice;
    float playbackSpeed = 1.0f;
    int trackCount = 0;
    char trackPaths[MAX_TRACKS][64]{};
    char trackNames[MAX_TRACKS][64]{};
};