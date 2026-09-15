#include "led_strip.h"

static CRGB stripLeds[AppConfig::Led::NUM_LEDS];

void initLedStrip() {
    Serial.printf("Initializing LED strip on GPIO %u (%u LEDs)...\n",
                  AppConfig::Led::PIN, AppConfig::Led::NUM_LEDS);
    FastLED.addLeds<WS2812B, AppConfig::Led::PIN, GRB>(stripLeds, AppConfig::Led::NUM_LEDS);
    FastLED.setBrightness(AppConfig::Led::DEFAULT_BRIGHT);
    clearLedStrip();
}

void clearLedStrip() {
    FastLED.clear();
    FastLED.show();
}

void runLedChase(const CRGB& color, uint8_t brightness, uint8_t stepDelayMs, int increment) {
    FastLED.setBrightness(brightness);
    for (uint16_t i = 0; i < AppConfig::Led::NUM_LEDS; i += increment) {
        FastLED.clear();
        stripLeds[i] = color;
        FastLED.show();
        delay(stepDelayMs);
    }
}
