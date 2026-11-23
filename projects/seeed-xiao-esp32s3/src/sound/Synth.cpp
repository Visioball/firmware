#include "Synth.h"
#include "../configuration/config.h"
#include <Arduino.h>
#include <math.h>

void Synth::update(float angularSpeed, float, float) {
    float maximumAngularSpeed = Config::Motion::MOTION_GAIN * DEG_TO_RAD;
    float normalizedSpeed = constrain(angularSpeed / maximumAngularSpeed, 0.0f, 1.0f);
    toneFrequency = 250.0f + normalizedSpeed * 350.0f;
}

void Synth::render(float* output, int sampleCount, float sampleRate, float volume) {
    float phaseStep = toneFrequency / sampleRate;
    constexpr float BEEP_GAIN = 0.15f;

    for (int i = 0; i < sampleCount; i++) {
        output[i] = (1.0f - 4.0f * fabsf(phase - 0.5f)) * volume * BEEP_GAIN;
        phase += phaseStep;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

float Synth::frequency() const {
    return toneFrequency;
}