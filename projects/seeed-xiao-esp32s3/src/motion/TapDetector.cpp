#include "TapDetector.h"
#include <math.h>

namespace {
constexpr uint32_t WARMUP_SAMPLES = 100;
constexpr uint32_t QUIET_TIME_MS = 120;
constexpr uint32_t TAP_COOLDOWN_MS = 300;
constexpr float TAP_THRESHOLD = 1.20f;
constexpr float RELEASE_THRESHOLD = 0.60f;
constexpr float AXIS_DOMINANCE = 1.40f;
}

TapGesture TapDetector::update(float accelX, float accelY, float accelZ, uint32_t now) {
    if (!initialized) {
        baselineX = previousX = accelX;
        baselineY = previousY = accelY;
        baselineZ = previousZ = accelZ;
        lastTapTime = now;
        initialized = true;
        return TapGesture::None;
    }

    if (warmupCount < WARMUP_SAMPLES) {
        warmupCount++;
        baselineX = 0.80f * baselineX + 0.20f * accelX;
        baselineY = 0.80f * baselineY + 0.20f * accelY;
        baselineZ = 0.80f * baselineZ + 0.20f * accelZ;
        previousX = accelX;
        previousY = accelY;
        previousZ = accelZ;
        return TapGesture::None;
    }

    baselineX = 0.90f * baselineX + 0.10f * accelX;
    baselineY = 0.90f * baselineY + 0.10f * accelY;
    baselineZ = 0.90f * baselineZ + 0.10f * accelZ;

    float highPassX = accelX - baselineX;
    float highPassY = accelY - baselineY;
    float highPassZ = accelZ - baselineZ;
    float jerkX = accelX - previousX;
    float jerkY = accelY - previousY;
    float jerkZ = accelZ - previousZ;
    previousX = accelX;
    previousY = accelY;
    previousZ = accelZ;

    float magnitudeX = max(fabsf(highPassX), fabsf(jerkX));
    float magnitudeY = max(fabsf(highPassY), fabsf(jerkY));
    float magnitudeZ = max(fabsf(highPassZ), fabsf(jerkZ));

    if (!armed) {
        float maximumMagnitude = max(magnitudeX, max(magnitudeY, magnitudeZ));
        if (maximumMagnitude <= RELEASE_THRESHOLD) {
            if (!quietTiming) {
                quietSince = now;
                quietTiming = true;
            } else if (now - quietSince >= QUIET_TIME_MS) {
                armed = true;
                quietTiming = false;
            }
        } else {
            quietTiming = false;
        }
        return TapGesture::None;
    }

    if (now - lastTapTime < TAP_COOLDOWN_MS) return TapGesture::None;

    if (magnitudeZ >= TAP_THRESHOLD &&
        magnitudeZ > AXIS_DOMINANCE * magnitudeX &&
        magnitudeZ > AXIS_DOMINANCE * magnitudeY) {
        disarm(now);
        return TapGesture::Z;
    }

    if (magnitudeX >= TAP_THRESHOLD &&
        magnitudeX > AXIS_DOMINANCE * magnitudeZ &&
        magnitudeX > AXIS_DOMINANCE * magnitudeY) {
        disarm(now);
        return (highPassX > 0.0f || jerkX > 0.0f)
            ? TapGesture::XPositive
            : TapGesture::XNegative;
    }

    return TapGesture::None;
}

void TapDetector::disarm(uint32_t now) {
    lastTapTime = now;
    armed = false;
    quietTiming = false;
}