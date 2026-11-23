#pragma once
#include <Arduino.h>

enum class TapGesture {
    None,
    Z,
    XPositive,
    XNegative
};

class TapDetector {
public:
    TapGesture update(float accelX, float accelY, float accelZ, uint32_t now);

private:
    void disarm(uint32_t now);

    bool initialized = false;
    bool armed = true;
    bool quietTiming = false;
    uint32_t warmupCount = 0;
    uint32_t lastTapTime = 0;
    uint32_t quietSince = 0;
    float baselineX = 0.0f;
    float baselineY = 0.0f;
    float baselineZ = 0.0f;
    float previousX = 0.0f;
    float previousY = 0.0f;
    float previousZ = 0.0f;
};