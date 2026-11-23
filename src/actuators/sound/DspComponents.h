#pragma once
#include <Arduino.h>
#include <esp_heap_caps.h>
#include "WavSource.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class AdsrEnvelope {
public:
    void setSampleRate(float value) { sampleRate = value; setTimes(0.02f, 0.1f, 0.7f, 0.3f); }
    void setTimes(float attack, float decay, float sustain, float release) {
        sustainLevel = constrain(sustain, 0.0f, 1.0f);
        attackStep = attack > 0.0001f ? 1.0f / (attack * sampleRate) : 1.0f;
        decayStep = decay > 0.0001f ? (1.0f - sustainLevel) / (decay * sampleRate) : 1.0f;
        releaseStep = release > 0.0001f ? sustainLevel / (release * sampleRate) : 1.0f;
    }
    void noteOn() { state = ATTACK; }
    float process() {
        switch (state) {
        case ATTACK: level += attackStep; if (level >= 1.0f) { level = 1.0f; state = DECAY; } break;
        case DECAY: level -= decayStep; if (level <= sustainLevel) { level = sustainLevel; state = SUSTAIN; } break;
        case SUSTAIN: level = sustainLevel; break;
        case RELEASE: level -= releaseStep; if (level <= 0.0f) { level = 0.0f; state = IDLE; } break;
        case IDLE: level = 0.0f; break;
        }
        return level;
    }

private:
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } state = IDLE;
    float sampleRate = 44100.0f;
    float level = 0.0f;
    float attackStep = 0.001f;
    float decayStep = 0.0005f;
    float sustainLevel = 0.7f;
    float releaseStep = 0.0005f;
};

class OnePoleFilter {
public:
    void setSampleRate(float value) { sampleRate = value; setCutoff(8000.0f); }
    void setCutoff(float value) {
        value = constrain(value, 20.0f, sampleRate * 0.48f);
        float dt = 1.0f / sampleRate;
        float rc = 1.0f / (2.0f * (float)M_PI * value);
        alpha = dt / (rc + dt);
    }
    float process(float input) { state += alpha * (input - state); return state; }

private:
    float sampleRate = 44100.0f;
    float alpha = 1.0f;
    float state = 0.0f;
};

class SampleVoice {
public:
    void init(const WavSource* source, float rate) {
        wav = source;
        sampleRate = rate;
        envelope.setSampleRate(rate);
        filterA.setSampleRate(rate);
        filterB.setSampleRate(rate);
    }
    void noteOn(float value, float cutoff = 12000.0f) { pitch = value; setCutoff(cutoff); envelope.noteOn(); }
    void setPitch(float value) { pitch = constrain(value, 0.05f, 4.0f); }
    void setTempo(float value) { tempo = constrain(value, 0.05f, 4.0f); }
    void setCutoff(float value) { filterA.setCutoff(value); filterB.setCutoff(value); }
    void setEnvelope(float attack, float decay, float sustain, float release) { envelope.setTimes(attack, decay, sustain, release); }
    void render(float* output, int count, float gain) {
        for (int i = 0; i < count; i++) {
            size_t total = wav->getSampleCount();
            size_t index = (size_t)phase;
            size_t next = (index + 1) % total;
            float fraction = phase - (float)index;
            float sample = ((float)wav->getSample(index) + fraction * (wav->getSample(next) - wav->getSample(index))) / 32768.0f;
            sample *= envelope.process();
            sample = filterB.process(filterA.process(sample));
            output[i] += sample * gain;
            phase += tempo * pitch * wav->getSampleRate() / sampleRate;
            if (phase >= total) phase = fmodf(phase, (float)total);
        }
    }

private:
    const WavSource* wav = nullptr;
    AdsrEnvelope envelope;
    OnePoleFilter filterA;
    OnePoleFilter filterB;
    float sampleRate = 44100.0f;
    float phase = 0.0f;
    float pitch = 1.0f;
    float tempo = 1.0f;
};

class DelayEffect {
public:
    ~DelayEffect() { free(buffer); }
    bool init(float rate, float milliseconds) {
        size = (size_t)(rate * milliseconds / 1000.0f) + 1024;
        delaySamples = (size_t)(rate * 300.0f / 1000.0f);
        buffer = (float*)heap_caps_calloc(size, sizeof(float), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!buffer) buffer = (float*)calloc(size, sizeof(float));
        return buffer != nullptr;
    }
    void setMix(float value) {
        mix = constrain(value, 0.0f, 0.70f);
        feedback = 0.20f + mix * 0.55f;
    }
    void process(float* samples, int count) {
        if (!buffer || mix <= 0.001f) return;
        for (int i = 0; i < count; i++) {
            size_t read = (write + size - delaySamples) % size;
            float echo = buffer[read];
            buffer[write] = samples[i] + echo * feedback;
            samples[i] = samples[i] * (1.0f - mix * 0.5f) + echo * mix;
            write = (write + 1) % size;
        }
    }

private:
    float* buffer = nullptr;
    size_t size = 0;
    size_t write = 0;
    size_t delaySamples = 0;
    float mix = 0.0f;
    float feedback = 0.3f;
};

class BitcrusherEffect {
public:
    void setIntensity(float value) {
        intensity = constrain(value, 0.0f, 1.0f);
        bits = 16.0f - intensity * 13.0f;
        downsample = 1 + (int)(intensity * 11.0f);
    }
    void process(float* samples, int count) {
        if (intensity <= 0.005f) return;
        float steps = powf(2.0f, bits - 1.0f);
        for (int i = 0; i < count; i++) {
            if (holdCounter++ >= downsample) {
                holdCounter = 0;
                heldSample = roundf(samples[i] * steps) / steps;
            }
            samples[i] = heldSample;
        }
    }

private:
    float intensity = 0.0f;
    float bits = 16.0f;
    int downsample = 1;
    int holdCounter = 0;
    float heldSample = 0.0f;
};