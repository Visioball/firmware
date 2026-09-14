#include "bus_manager.h"
#include <Wire.h>
#include <SPI.h>
#include "../config/app_config.h"

void initI2C() {
    Serial.printf("[BusManager] Initializing I2C (SDA: %u, SCL: %u @ %u Hz)...\n",
                  AppConfig::I2C::SDA_PIN,
                  AppConfig::I2C::SCL_PIN,
                  AppConfig::I2C::FREQUENCY);
    Wire.begin(AppConfig::I2C::SDA_PIN, AppConfig::I2C::SCL_PIN, AppConfig::I2C::FREQUENCY);
    Serial.println("[BusManager] I2C bus initialized.");
}

void initSPI() {
    Serial.printf("[BusManager] Initializing default SPI (SCK: %u, MISO: %u, MOSI: %u)...\n",
                  AppConfig::SPI::Ethernet::SCK_PIN,
                  AppConfig::SPI::Ethernet::MISO_PIN,
                  AppConfig::SPI::Ethernet::MOSI_PIN);
    SPI.begin(AppConfig::SPI::Ethernet::SCK_PIN,
              AppConfig::SPI::Ethernet::MISO_PIN,
              AppConfig::SPI::Ethernet::MOSI_PIN);
    Serial.println("[BusManager] SPI bus initialized.");
}

void initBuses() {
    initI2C();
    initSPI();
}

