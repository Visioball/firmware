#pragma once
#include <Arduino.h>

// =============================================================================
// Compile-time configuration. config.json is not loaded by the firmware.
// =============================================================================

namespace Config {
    // Board-specific pin definitions
    namespace Pins {
#if defined(VISIOBALL_ESP32_S3_DEVKITC1_V11)
        constexpr uint8_t LED_DATA_1   = 40;
        constexpr uint8_t LED_DATA_2   = 39;

        // MAX98357A I2S amplifier
        constexpr uint8_t MAX98357_SD  = 4;
        constexpr uint8_t I2S_DATA     = 5;
        constexpr uint8_t I2S_BCK      = 6;
        constexpr uint8_t I2S_WS       = 7;

        // BMI270 SPI
        constexpr uint8_t BMI270_CS    = 10;
        constexpr uint8_t SPI_MOSI     = 11;
        constexpr uint8_t SPI_SCK      = 12;
        constexpr uint8_t SPI_MISO     = 13;
#else
        // Seeed XIAO ESP32S3
        constexpr uint8_t LED_DATA      = D5;

        // I2S Audio
        constexpr uint8_t I2S_BCK       = 2;    // D1
        constexpr uint8_t I2S_WS        = 1;    // D0
        constexpr uint8_t I2S_DATA      = 3;    // D2

        // BMI270 SPI Motion Sensor
        constexpr uint8_t BMI270_CS     = D3;   // GPIO 4
        constexpr uint8_t SPI_SCK       = D8;   // GPIO 7/8
        constexpr uint8_t SPI_MOSI      = D9;   // GPIO 8/9
        constexpr uint8_t SPI_MISO      = D10;  // GPIO 9/10
        constexpr uint8_t BMI270_INT1   = D6;   // GPIO 43
        constexpr uint8_t BMI270_INT2   = D7;   // GPIO 44
#endif

        // MPU9250 I2C Motion Sensor
        constexpr uint8_t MPU9250_SDA   = 16;
        constexpr uint8_t MPU9250_SCL   = 17;
    }

    // Audio Engine & DSP Configuration
    namespace Sound {
        constexpr float MIN_VOLUME      = 1.0f;     // 1% minimum volume
        constexpr float DEFAULT_VOLUME  = 1.0f;     // 1% startup volume
        constexpr float MAX_VOLUME      = 100.0f;
        constexpr int   DEFAULT_BLOCK_SIZE = 128;   // 128 or 256 samples
        constexpr int   SAMPLE_RATE     = 44100;
        constexpr int   NUM_VOICES      = 8;
        constexpr float WAV_MAX_ROLLING_SPEED = 24.0f; // rad/s: 3 m/s at 12.5 cm radius
    }

    namespace Light {
        constexpr uint16_t LED_COUNT = 18;
    }

    // Motion Sensor Parameters
    namespace Motion {
        constexpr float MOTION_GAIN          = 180.0f;
        constexpr float STATIONARY_THRESHOLD = 0.4f;
    }
}

// Backward-compatible macro defines
#define PIN_I2S_BCK          Config::Pins::I2S_BCK
#define PIN_I2S_WS           Config::Pins::I2S_WS
#define PIN_I2S_DATA         Config::Pins::I2S_DATA
#define PIN_BMI270_CS        Config::Pins::BMI270_CS
#define PIN_SPI_SCK          Config::Pins::SPI_SCK
#define PIN_SPI_MOSI         Config::Pins::SPI_MOSI
#define PIN_SPI_MISO         Config::Pins::SPI_MISO
#if !defined(VISIOBALL_ESP32_S3_DEVKITC1_V11)
#define PIN_BMI270_INT1      Config::Pins::BMI270_INT1
#define PIN_BMI270_INT2      Config::Pins::BMI270_INT2
#endif
