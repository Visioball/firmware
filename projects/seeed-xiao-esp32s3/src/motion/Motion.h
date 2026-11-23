#pragma once
#include <Arduino.h>
#include "Bmi270Sensor.h"
#include "MotionTarget.h"
#include "TapDetector.h"
#include "../spatial/Quaternion.h"

class Motion {
public:
    void bind(MotionTarget* target);
    void begin(float stationaryThreshold);
    void update(bool tapGesturesEnabled = true);
    void enableDataReadyInterrupt(bool enable);
    bool connected() const;
    float angularSpeed() const;
    float angularVelocityZ() const;
    const MotionSample& sample() const { return sensor.sample(); }
    Quaternion getOrientation() const {
        return Quaternion::fromTilt(sample().roll, sample().pitch);
    }

private:
    void dispatchTap(TapGesture gesture);

    Bmi270Sensor sensor;
    TapDetector tapDetector;
    MotionTarget* target = nullptr;
    float stationaryThreshold = 0.4f;
    uint32_t lastMotionTime = 0;
    uint32_t lastHeartbeatCycleTime = 0;
};