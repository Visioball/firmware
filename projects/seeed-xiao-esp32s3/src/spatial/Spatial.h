#pragma once
#include "../motion/Motion.h"

class Spatial {
public:
    explicit Spatial(const Motion& motion) : motion(motion) {}

    Vector3 getWorldPosition(const Vector3& localPosition) const {
        return motion.getOrientation().rotate(localPosition);
    }

    float getUpValue(const Vector3& localPosition) const {
        return getWorldPosition(localPosition).z;
    }

    bool isUpperHalf(const Vector3& localPosition) const {
        return getUpValue(localPosition) >= 0.0f;
    }

    float angularVelocityZ() const { return motion.angularVelocityZ(); }

private:
    const Motion& motion;
};
