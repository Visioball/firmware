#include "WavDSP.h"
#include <Arduino.h>
#include <math.h>
#include "../configuration/config.h"

bool WavDSP::initialize(const char* wavPath) {
    if (!source.loadFromFile(wavPath, true)) return false;

    float rate = (float)source.getSampleRate();
    voice.init(&source, rate);
    voice.setEnvelope(0.005f, 0.05f, 1.0f, 0.2f);
    voice.noteOn(1.0f, 12000.0f);
    if (!delay.init(rate, Config::Sound::DELAY_TIME_MS)) {
        Serial.println("[SOUND] Unable to allocate WAV delay buffer");
        return false;
    }
    return true;
}

void WavDSP::start() {
    voice.noteOn(1.0f, 12000.0f);
}

void WavDSP::update(float, float roll, float pitch) {
    pitchMultiplier = powf(2.0f, constrain(roll / 45.0f, -1.0f, 1.0f));

    float tilt = constrain(pitch / 45.0f, -1.0f, 1.0f);
    if (tilt < -0.05f) {
        float delayAmount = constrain((-tilt - 0.05f) / 0.95f, 0.0f, 1.0f);
        delay.setMix(delayAmount * 0.65f);
        bitcrusher.setIntensity(0.0f);
    } else if (tilt > 0.05f) {
        float crushAmount = constrain((tilt - 0.05f) / 0.95f, 0.0f, 1.0f);
        bitcrusher.setIntensity(crushAmount);
        delay.setMix(0.0f);
    } else {
        delay.setMix(0.0f);
        bitcrusher.setIntensity(0.0f);
    }
}

void WavDSP::render(float* output, int sampleCount, float, float volume) {
    voice.setPitch(pitchMultiplier);
    voice.setTempo(1.0f);
    voice.setCutoff(16000.0f);
    voice.render(output, sampleCount, 0.9f);
    delay.process(output, sampleCount);
    bitcrusher.process(output, sampleCount);

    for (int i = 0; i < sampleCount; i++) {
        output[i] = constrain(output[i] * volume, -1.0f, 1.0f);
    }
}

float WavDSP::sampleRate() const {
    return (float)source.getSampleRate();
}

const char* WavDSP::storageType() const {
    return source.isLoadedInPsram() ? "PSRAM" : (source.isFlashMapped() ? "FLASH-MAPPED" : "NONE");
}

const char* WavDSP::loadLog() const {
    return source.getLoadLog();
}