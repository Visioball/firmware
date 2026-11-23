#include "Light.h"
#include <cmath>

Light::Light(const Spatial& spatial)
    : spatial(spatial),
      output(Config::Light::LED_COUNT, Config::Pins::LED_DATA, NEO_GRBW + NEO_KHZ800) {
    for (size_t id = 0; id < pixels.size(); ++id) pixels[id].id = id;
}

void Light::begin() {
    output.begin();
    lastMotionAt = millis();
    render(true);
}

void Light::update() {
    const uint32_t now = millis();
    const uint32_t elapsed = now - lastMotionAt;
    lastMotionAt = now;
    const float velocity = spatial.angularVelocityZ();
    if (elapsed <= 100 && std::fabs(velocity) > 0.03f) {
        intensity = constrain(intensity + velocity * elapsed *
                              (255.0f / (1000.0f * PI)), 0.0f, 255.0f);
    }
    render();
}

void Light::setEnabled(bool value) {
    enabled = value;
}

void Light::setLevel(float value) {
    if (!std::isfinite(value) || value < 0.0f || value > 255.0f) {
        Serial.println("[LIGHT] Intensity must be between 0 and 255");
        return;
    }
    intensity = value;
}

void Light::apply(const LightSettings& settings) {
    enabled = settings.enabled;
    intensity = settings.intensity;
    solid.setColor({settings.red, settings.green, settings.blue});
    activeEffect = &solid;
    render();
}

bool Light::setEffect(ILightEffect& effect) {
    if (effect.requiresPositions()) {
        for (const LED& led : pixels) {
            if (!led.positioned) {
                Serial.println("[LIGHT] Set all 72 LED positions before using a spatial effect");
                return false;
            }
        }
    }
    activeEffect = &effect;
    return true;
}

bool Light::setPosition(size_t id, Vector3 position) {
    if (id >= pixels.size() || !std::isfinite(position.x) ||
        !std::isfinite(position.y) || !std::isfinite(position.z) ||
        (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f)) {
        Serial.println("[LIGHT] Invalid LED position or ID");
        return false;
    }
    pixels[id].position = position;
    pixels[id].positioned = true;
    return true;
}

void Light::render(bool force) {
    const float brightness = enabled ? intensity / 255.0f : 0.0f;
    bool changed = force;
    for (LED& led : pixels) {
        led.color = activeEffect->update(led, spatial);
        const uint32_t value = output.Color(led.color.red * brightness,
                              led.color.green * brightness, led.color.blue * brightness, 0);
        if (output.getPixelColor(led.id) != value) {
            output.setPixelColor(led.id, value);
            changed = true;
        }
    }
    if (changed) output.show();
}
