#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "../config/app_config.h"

void initLedStrip();
void runLedChase(const CRGB& color = CRGB::White, 
                 uint8_t brightness = AppConfig::Led::DEFAULT_BRIGHT, 
                 uint8_t stepDelayMs = AppConfig::Led::CHASE_DELAY_MS);
void clearLedStrip();
