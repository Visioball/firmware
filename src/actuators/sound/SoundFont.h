#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <LittleFS.h>
#include "DspComponents.h"
#include "ISoundEffect.h"

#pragma pack(push, 1)
struct SF2Phdr {
    char presetName[20];
    uint16_t preset;
    uint16_t bank;
    uint16_t presetBagNdx;
    uint32_t library;
    uint32_t genre;
    uint32_t morphology;
};

struct SF2Bag { uint16_t genNdx; uint16_t modNdx; };
struct SF2Gen { uint16_t genOper; int16_t genAmount; };

struct SF2Inst {
    char instName[20];
    uint16_t instBagNdx;
};

struct SF2Shdr {
    char sampleName[20];
    uint32_t start;
    uint32_t end;
    uint32_t startLoop;
    uint32_t endLoop;
    uint32_t sampleRate;
    uint8_t originalPitch;
    int8_t pitchCorrection;
    uint16_t sampleLink;
    uint16_t sampleType;
};
#pragma pack(pop)

struct SoundFontZone {
    uint8_t loKey = 0;
    uint8_t hiKey = 127;
    uint8_t loVel = 0;
    uint8_t hiVel = 127;
    uint8_t rootKey = 60;
    int16_t fineTune = 0;
    int16_t coarseTune = 0;
    uint32_t sampleStart = 0;
    uint32_t sampleEnd = 0;
    uint32_t loopStart = 0;
    uint32_t loopEnd = 0;
    uint32_t sampleRate = 44100;
    uint8_t loopMode = 0;
    float pan = 0.0f;
    float attenuation = 1.0f;
    float attackSec = 0.001f;
    float holdSec = 0.0f;
    float decaySec = 5.0f;
    float sustainLevel = 0.0f;
    float releaseSec = 0.4f;
    float initialFilterCutoff = 18000.0f;
};

struct SoundFontVoice {
    bool active = false;
    int note = -1;
    float velocity = 0.0f;
    uint32_t sampleStart = 0;
    uint32_t sampleEnd = 0;
    uint32_t loopStart = 0;
    uint32_t loopEnd = 0;
    uint8_t loopMode = 0;
    double samplePosition = 0.0;
    double pitchRatio = 1.0;
    float voiceGain = 1.0f;
    float pan = 0.0f;

    enum EnvState { ENV_IDLE, ENV_ATTACK, ENV_HOLD, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } envState = ENV_IDLE;
    float envLevel = 0.0f;
    float attackStep = 0.01f;
    uint32_t holdSamplesLeft = 0;
    float decayFactor = 0.9999f;
    float sustainLevel = 0.0f;
    float releaseFactor = 0.999f;
    uint32_t age = 0;
    OnePoleFilter filter;
};

class SoundFont final : public ISoundEffect {
public:
    SoundEffectType type() const override { return SoundEffectType::SoundFont; }
    void update(float movement, float roll, float pitch) override;
    void start() override { triggerNote(); }
    static constexpr int MAX_SOUNDFONTS = 16;
    static constexpr int MAX_ZONES = 64;
    static constexpr int MAX_VOICES = 16;

    ~SoundFont() { clear(); }

    void clear() {
        isBusyLoading = true;
        allNotesOff();
        sampleData = nullptr;
        fileSize = 0;
        zoneCount = 0;
        if (fileData) {
            free(fileData);
            fileData = nullptr;
        }
        isBusyLoading = false;
    }

    int scanFiles();
    int count() const { return soundFontCount; }
    const char* name(int index) const {
        return index >= 0 && index < soundFontCount ? soundFontNames[index] : "";
    }
    bool select(int index);

    bool load(const char* path) {
        if (!path || !path[0]) return false;
        File file = LittleFS.open(path, "r");
        if (!file) return false;
        size_t newSize = file.size();
        if (newSize < 100) { file.close(); return false; }

        isBusyLoading = true;
        allNotesOff();
        vTaskDelay(pdMS_TO_TICKS(10));

        if (fileData) {
            free(fileData);
            fileData = nullptr;
            sampleData = nullptr;
            fileSize = 0;
            zoneCount = 0;
        }

        uint8_t* newData = (uint8_t*)heap_caps_malloc(newSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!newData) newData = (uint8_t*)malloc(newSize);
        if (!newData) {
            file.close();
            isBusyLoading = false;
            return false;
        }

        const size_t CHUNK_SIZE = 4096;
        uint8_t* chunk = (uint8_t*)heap_caps_malloc(CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!chunk) chunk = (uint8_t*)malloc(CHUNK_SIZE);
        if (!chunk) {
            free(newData);
            file.close();
            isBusyLoading = false;
            return false;
        }

        size_t bytesRead = 0;
        bool readOk = true;
        while (bytesRead < newSize) {
            size_t wanted = min(CHUNK_SIZE, newSize - bytesRead);
            size_t received = file.read(chunk, wanted);
            if (received == 0) {
                readOk = false;
                break;
            }
            memcpy(newData + bytesRead, chunk, received);
            bytesRead += received;
        }
        free(chunk);
        file.close();

        if (!readOk || bytesRead != newSize) {
            free(newData);
            isBusyLoading = false;
            return false;
        }

        fileData = newData;
        fileSize = newSize;
        if (!parseSoundFont()) {
            free(newData);
            fileData = nullptr;
            fileSize = 0;
            sampleData = nullptr;
            zoneCount = 0;
            isBusyLoading = false;
            return false;
        }

        for (int voice = 0; voice < MAX_VOICES; voice++) {
            voices[voice].active = false;
            voices[voice].envState = SoundFontVoice::ENV_IDLE;
        }
        isBusyLoading = false;
        return zoneCount > 0;
    }

    void setTempo(float value) { tempo = constrain(value, 0.5f, 20.0f); }
    void setPitch(float value) { pitch = constrain(value, 0.25f, 4.0f); }
    void setMotionLevel(float value) {
        motionLevel = constrain(value, 0.0f, 1.0f);
        velocity = 0.40f + motionLevel * 0.55f;
        masterFilter.setCutoff(4000.0f + motionLevel * 14000.0f);
    }

    void noteOn(int midiNote, float noteVelocity = 0.8f) {
        if (isBusyLoading || !sampleData || zoneCount == 0) return;
        midiNote = constrain(midiNote, 0, 127);
        int velocityValue = constrain((int)(noteVelocity * 127), 1, 127);

        for (size_t zoneIndex = 0; zoneIndex < zoneCount; zoneIndex++) {
            const SoundFontZone& zone = zones[zoneIndex];
            if (midiNote < zone.loKey || midiNote > zone.hiKey) continue;
            if (velocityValue < zone.loVel || velocityValue > zone.hiVel) continue;

            int voiceIndex = allocateVoice();
            if (voiceIndex < 0) continue;
            SoundFontVoice& voice = voices[voiceIndex];
            voice.active = true;
            voice.note = midiNote;
            voice.velocity = noteVelocity;
            voice.sampleStart = zone.sampleStart;
            voice.sampleEnd = zone.sampleEnd;
            voice.loopStart = zone.loopStart;
            voice.loopEnd = zone.loopEnd;
            voice.loopMode = zone.loopMode;
            voice.samplePosition = 0.0;

            double semitoneOffset = midiNote - (double)zone.rootKey +
                ((double)zone.coarseTune + (double)zone.fineTune / 100.0);
            voice.pitchRatio = ((double)zone.sampleRate / currentSampleRate) * pow(2.0, semitoneOffset / 12.0);
            voice.voiceGain = noteVelocity * zone.attenuation;
            voice.pan = zone.pan;
            voice.envState = SoundFontVoice::ENV_ATTACK;
            voice.envLevel = 0.0f;
            voice.attackStep = zone.attackSec > 0.0005f
                ? 1.0f / (zone.attackSec * currentSampleRate) : 1.0f;
            voice.holdSamplesLeft = (uint32_t)(zone.holdSec * currentSampleRate);
            float decayTime = zone.decaySec > 0.05f ? zone.decaySec : 0.05f;
            voice.decayFactor = expf(-6.907755f / (decayTime * currentSampleRate));
            voice.sustainLevel = zone.sustainLevel;
            float releaseTime = zone.releaseSec > 0.02f ? zone.releaseSec : 0.02f;
            voice.releaseFactor = expf(-6.907755f / (releaseTime * currentSampleRate));
            voice.age = 0;
            voice.filter.setSampleRate(currentSampleRate);
            voice.filter.setCutoff(zone.initialFilterCutoff);
        }
    }

    void allNotesOff() {
        for (int voice = 0; voice < MAX_VOICES; voice++) {
            if (voices[voice].active) voices[voice].envState = SoundFontVoice::ENV_RELEASE;
        }
    }

    void triggerNote() {
        if (!isBusyLoading && sampleData && zoneCount > 0) noteOn(currentMidi(), velocity);
    }

    void triggerHeartbeat(float intensity = 0.5f) override {
        if (!isBusyLoading && sampleData && zoneCount > 0) {
            noteOn(48, constrain(0.18f + intensity * 0.22f, 0.12f, 0.50f));
        }
    }

    void render(float* output, int count, float outputRate, float volume) override;

private:
    static float timecentsToSeconds(int16_t timecents) {
        return timecents <= -11950 ? 0.001f : powf(2.0f, (float)timecents / 1200.0f);
    }

    size_t findChunk(const char* name, size_t startOffset = 0) const;
    size_t findListChunk(const char* listType) const;
    size_t findSubChunk(size_t parentOffset, size_t parentSize, const char* name, uint32_t* outSize = nullptr) const;
    bool parseSoundFont();
    int allocateVoice();
    void playNextMotionNote();
    int currentMidi() const;

    uint8_t* fileData = nullptr;
    const int16_t* sampleData = nullptr;
    size_t fileSize = 0;
    size_t zoneCount = 0;
    volatile bool isBusyLoading = false;
    SoundFontZone zones[MAX_ZONES];
    SoundFontVoice voices[MAX_VOICES];
    char soundFontPaths[MAX_SOUNDFONTS][64] = {};
    char soundFontNames[MAX_SOUNDFONTS][64] = {};
    int soundFontCount = 0;
    uint32_t samplesUntilNextNote = 0;
    OnePoleFilter masterFilter;
    float currentSampleRate = 44100.0f;
    float tempo = 1.0f;
    float pitch = 1.0f;
    float motionLevel = 0.0f;
    float velocity = 0.6f;
};