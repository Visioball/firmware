#include "hardware_diagnostics.h"
#include "../config/app_config.h"

void logSystemInfo() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("         ESP32-S3-TEST Startup          ");
    Serial.println("========================================");
    Serial.printf("Chip Model:     %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency:  %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Heap Memory:    %u KB total, %u KB free\n",
                  ESP.getHeapSize() / 1024, ESP.getFreeHeap() / 1024);
    Serial.printf("Flash Size:     %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM Memory:   %u KB total, %u KB free (%s)\n",
                  ESP.getPsramSize() / 1024, ESP.getFreePsram() / 1024,
                  psramFound() ? "detected" : "not detected");
    Serial.println("----------------------------------------");
}

void runGpioWalkthrough() {
    Serial.println("Starting sequential GPIO walkthrough...");

    for (size_t i = 0; i < AppConfig::Diagnostics::TEST_PIN_COUNT; ++i) {
        const uint8_t pin = AppConfig::Diagnostics::TEST_PINS[i];

        Serial.printf("  [GPIO %2u] -> Output LOW -> HIGH -> LOW\n", pin);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        delay(20);
        digitalWrite(pin, HIGH);
        delay(20);
        digitalWrite(pin, LOW);
        pinMode(pin, INPUT);
    }

    Serial.println("GPIO walkthrough complete.\n");
}

void runHardwareDiagnostics() {
    logSystemInfo();
    runGpioWalkthrough();
}

