#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <atomic>
#include "Synth.h"
#include "SoundFont.h"
#include "WavDSP.h"
#include "Beep.h"
#include "ISoundOutput.h"
#include "../configuration/config.h"
#include "../IActuator.h"

struct BenchmarkStats {
    uint32_t triggerCount;
    int64_t latencyTotalUs;
    int64_t latencyMaxUs;
    int64_t latencyAvgUs;
    int64_t lastIsrToStartUs;
    int64_t lastStartToBufUs;
    int64_t lastTotalLatencyUs;
    uint32_t blockCount;
    int64_t blockDurationTotalUs;
    int64_t blockDurationMaxUs;
    int64_t blockDurationAvgUs;
    float cpuLoadPercent;
    uint32_t underruns;
    int16_t peakPcm;
    float avgPcm;
    size_t totalBytesWritten;
    int blockSize;
    float sampleRate;
    int64_t idealBlockTimeUs;
};

class Sound final : public IActuator {
public:
    explicit Sound(ISoundOutput& output);
    ~Sound();
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    bool init(const char* wavPath, int blockSize = Config::Sound::DEFAULT_BLOCK_SIZE);
    int scanSoundFonts();
    int getSoundFontCount() const;
    const char* getSoundFontName(int index) const;
    bool selectSoundFontIndex(int index);
    bool selectEffectIndex(int index);

    void setBlockSize(int newBlockSize);
    void triggerMotionInterrupt(int64_t isrTimestamp);
    void renderTaskLoop();
    BenchmarkStats getStats();
    void resetPeriodicStats();
    void resetLatencyMax();

    void setMasterVolume(float volumePercent);
    float getMasterVolume() const;
    void updateMotion(float movement, float roll, float pitch);
    void triggerHeartbeat(float intensity = 0.5f);
    void onTapZ();
    void onTapX(int direction);
    void update() override;
    void setEnabled(bool enabled) override { this->enabled = enabled; }
    void setLevel(float volumePercent) override { setMasterVolume(volumePercent); }

    void setEffect(ISoundEffect& effect);
    void setEffectType(SoundEffectType type);
    SoundEffectType getEffectType() const;
    float getSynthFrequency() const;
    const char* getWavStorage() const;
    const char* getWavLoadLog() const;
    int getBlockSize() const;

private:
    void renderAudioBlock(float* output, int sampleCount);

    WavDSP wavDsp;
    SoundFont soundFont;
    Synth synth;
    Beep beep;
    ISoundEffect* activeEffect = &synth;
    ISoundOutput& output;
    Speakers speakers = axisSpeakers();
    SemaphoreHandle_t effectMutex = nullptr;

    std::atomic<int> currentBlockSize;
    float* blockBuffer;
    std::atomic<SoundEffectType> currentMode;
    int tapCycleIndex = -1;
    int pendingTapCycleIndex = -1;
    bool tapTransitionPending = false;
    uint32_t tapTransitionStartedAt = 0;
    std::atomic<float> masterVolume;
    std::atomic<bool> enabled{true};

    std::atomic<bool> pendingTrigger;
    volatile int64_t currentInterruptTimestamp;
    volatile int64_t currentAudioStartTimestamp;
    BenchmarkStats stats;
    portMUX_TYPE statsMux;
    TaskHandle_t audioTaskHandle;
    std::atomic<bool> running;
};