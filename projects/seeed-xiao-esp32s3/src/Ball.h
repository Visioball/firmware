#pragma once
#include "sound/Sound.h"
#include "light/Light.h"
#include "motion/Motion.h"
#include "debug/DebugLogger.h"
#include "configuration/RuntimeConfiguration.h"

class Ball final : public MotionTarget {
public:
    Ball(Sound& sound, Light& light, Motion& motion, DebugLogger& debug)
        : sound(sound), light(light), motion(motion), debug(debug) {}
    bool begin();
    void update();
    void updateMotion(float movement, float roll, float pitch) override;
    void triggerHeartbeat(float intensity) override;
    void onTapZ() override;
    void onTapX(int direction) override;

private:
    void applyReceivedConfiguration();

    Sound& sound;
    Light& light;
    Motion& motion;
    DebugLogger& debug;
    LightSettings lightSettings;
};
