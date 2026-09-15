#pragma once
#include <Arduino.h>

// =============================================================================
// Board & Serial Settings
// =============================================================================
namespace AppConfig {
    constexpr uint32_t SERIAL_BAUD_RATE   = 115200;
    constexpr uint32_t STARTUP_DELAY_MS   = 5000;

    // =========================================================================
    // LED Strip Configuration
    // =========================================================================
    namespace Led {
        constexpr uint8_t  PIN            = 18;
        constexpr uint16_t NUM_LEDS       = 64;
        constexpr uint8_t  DEFAULT_BRIGHT = 10;
        constexpr uint8_t  CHASE_DELAY_MS = 100;
    }

    // =========================================================================
    // Onboard Status LED Configuration (WS2812 on GPIO 21)
    // =========================================================================
    namespace OnboardLed {
        constexpr uint8_t  PIN               = 21;
        constexpr uint16_t NUM_LEDS          = 1;
        constexpr uint32_t BLINK_INTERVAL_MS = 500;
    }

    // =========================================================================
    // I2C Bus Configuration
    // =========================================================================
    namespace I2C {
        constexpr uint8_t  SDA_PIN   = 48;
        constexpr uint8_t  SCL_PIN   = 47;
        constexpr uint32_t FREQUENCY = 100000; // 100 kHz Standard Mode
    }

    // =========================================================================
    // SPI Bus Configuration
    // =========================================================================
    namespace SPI {
        // MicroSD (TF Card) SPI Bus
        namespace SD {
            constexpr uint8_t CS_PIN   = 4;
            constexpr uint8_t MISO_PIN = 5;
            constexpr uint8_t MOSI_PIN = 6;
            constexpr uint8_t SCK_PIN  = 7;
        }

        // W5500 Ethernet SPI Bus
        namespace Ethernet {
            constexpr uint8_t RST_PIN  = 9;
            constexpr uint8_t INT_PIN  = 10;
            constexpr uint8_t MOSI_PIN = 11;
            constexpr uint8_t MISO_PIN = 12;
            constexpr uint8_t SCK_PIN  = 13;
            constexpr uint8_t CS_PIN   = 14;
        }
    }

    // =========================================================================
    // WiFi Configuration
    // =========================================================================
    namespace WiFi {
        constexpr const char* SSID              = "Your_WiFi_SSID";
        constexpr const char* PASSWORD          = "Your_WiFi_Password";
        constexpr uint32_t    CONNECT_TIMEOUT_MS = 10000;
        constexpr const char* AP_SSID           = "ESP32-S3-TEST-AP";
        constexpr const char* AP_PASSWORD       = "12345678";
    }

    // =========================================================================
    // BLE Configuration
    // =========================================================================
    namespace Ble {
        constexpr const char* DEVICE_NAME       = "ESP32-S3-TEST";
        constexpr const char* SERVICE_UUID      = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
        constexpr const char* CHAR_STATUS_UUID  = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
        constexpr const char* CHAR_COMMAND_UUID = "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e";
    }

    // =========================================================================
    // GPIO Diagnostics (Safe Exposed Header Pins on ESP32-S3-ETH)
    // =========================================================================
    namespace Diagnostics {
        constexpr uint8_t TEST_PINS[] = {
            1, 2, 3, 15, 16, 17,
            38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
        };
        constexpr size_t TEST_PIN_COUNT = sizeof(TEST_PINS) / sizeof(TEST_PINS[0]);
    }
}
