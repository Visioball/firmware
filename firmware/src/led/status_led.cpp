#include "status_led.h"
#include <FastLED.h>
#include "../config/app_config.h"

static CRGB          statusLed[AppConfig::OnboardLed::NUM_LEDS];
static bool          statusLedState = false;
static unsigned long lastToggleTime = 0;
static uint32_t      blinkIntervalMs = AppConfig::OnboardLed::BLINK_INTERVAL_MS;

void initStatusLed() {
    FastLED.addLeds<WS2812B, AppConfig::OnboardLed::PIN, GRB>(statusLed, AppConfig::OnboardLed::NUM_LEDS);
    statusLed[0] = CRGB::Black;
    FastLED.show();
}

void updateStatusLed() {
    unsigned long now = millis();
    if (now - lastToggleTime >= blinkIntervalMs) {
        lastToggleTime = now;
        statusLedState = !statusLedState;
        statusLed[0] = statusLedState ? CRGB::White : CRGB::Black;
        FastLED.show();
    }
}

void setStatusLedBlinkInterval(uint32_t intervalMs) {
    blinkIntervalMs = intervalMs;
}

