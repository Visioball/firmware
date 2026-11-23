#pragma once

class MotionTarget {
public:
    virtual ~MotionTarget() = default;
    virtual void updateMotion(float movement, float roll, float pitch) = 0;
    virtual void triggerHeartbeat(float intensity = 0.5f) {}
    virtual void onTapZ() {}
    virtual void onTapX(int direction) {}
};