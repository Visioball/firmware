#pragma once
#include <array>
#include <Adafruit_NeoPixel.h>
#include "../IActuator.h"
#include "../configuration/config.h"
#include "../spatial/Spatial.h"
#include "LightSettings.h"
#include "Solid.h"

class Light final : public IActuator {
public:
    explicit Light(const Spatial& spatial);
    Light(const Light&) = delete;
    Light& operator=(const Light&) = delete;
    void begin();
    void update() override;
    void setEnabled(bool enabled) override;
    void setLevel(float intensity) override;
    void apply(const LightSettings& settings);
    bool setEffect(ILightEffect& effect);
    bool setPosition(size_t id, Vector3 position);
    const std::array<LED, Config::Light::LED_COUNT>& leds() const { return pixels; }

private:
    void render(bool force = false);

    const Spatial& spatial;
    std::array<LED, Config::Light::LED_COUNT> pixels;
    Adafruit_NeoPixel output;
    Solid solid;
    ILightEffect* activeEffect = &solid;
    bool enabled = true;
    float intensity = 64.0f;
    uint32_t lastMotionAt = 0;
};
