#include <Arduino.h>
#include "config/app_config.h"
#include "hardware/hardware_diagnostics.h"
#include "hardware/bus_manager.h"
#include "led/led_strip.h"
#include "led/status_led.h"
#include "wifi/wifi_service.h"
#include "ble/ble_service.h"

void setup() {
    Serial.begin(AppConfig::SERIAL_BAUD_RATE);
    const unsigned long serialStart = millis();
    while (!Serial && (millis() - serialStart < 2000)) {
        delay(10);
    }

    delay(AppConfig::STARTUP_DELAY_MS);

    runHardwareDiagnostics();
    initBuses();
    initStatusLed();
    initLedStrip();
    initWiFi();
    initBle();

    Serial.println("\n[ESP32-S3-TEST] All modules initialized. Entering main loop...\n");
}

void loop() {
    updateStatusLed();
    for (int i = 1; i < 4; i++){
        switch (i) {
        case 1:
            runLedChase(CRGB::Red, AppConfig::Led::DEFAULT_BRIGHT, AppConfig::Led::CHASE_DELAY_MS, i);
            break;
        case 2:
            runLedChase(CRGB::Green, AppConfig::Led::DEFAULT_BRIGHT, AppConfig::Led::CHASE_DELAY_MS, i);
            break;
        case 3:
            runLedChase(CRGB::Blue, AppConfig::Led::DEFAULT_BRIGHT, AppConfig::Led::CHASE_DELAY_MS, i);
            break;
        }
    }
}


