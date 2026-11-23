#include "Motion.h"

void Motion::bind(MotionTarget* value) {
    target = value;
}

void Motion::begin(float threshold) {
    stationaryThreshold = threshold;
    sensor.begin();
}

void Motion::update(bool tapGesturesEnabled) {
    sensor.update();
    if (!target) return;

    const MotionSample& sample = sensor.sample();
    target->updateMotion(sample.angularSpeed, sample.roll, sample.pitch);
    TapGesture gesture = tapDetector.update(sample.accelX, sample.accelY, sample.accelZ, millis());
    if (tapGesturesEnabled) dispatchTap(gesture);

    uint32_t now = millis();
    if (sample.angularSpeed > stationaryThreshold) {
        lastMotionTime = now;
    } else if (now - lastMotionTime >= 300 && now - lastHeartbeatCycleTime >= 1000) {
        lastHeartbeatCycleTime = now;
        target->triggerHeartbeat(0.5f);
    }
}

void Motion::enableDataReadyInterrupt(bool enable) {
    sensor.enableDataReadyInterrupt(enable);
}

bool Motion::connected() const {
    return sensor.connected();
}

float Motion::angularSpeed() const {
    return sensor.sample().angularSpeed;
}

float Motion::angularVelocityZ() const {
    return sensor.sample().angularVelocityZ;
}

void Motion::dispatchTap(TapGesture gesture) {
    switch (gesture) {
    case TapGesture::Z:
        target->onTapZ();
        break;
    case TapGesture::XPositive:
        target->onTapX(1);
        break;
    case TapGesture::XNegative:
        target->onTapX(-1);
        break;
    case TapGesture::None:
        break;
    }
}