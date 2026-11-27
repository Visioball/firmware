#include "Bmi270Sensor.h"
#include "../configuration/config.h"
#include <SPI.h>

namespace {
constexpr uint32_t SPI_CLOCK_FREQUENCY = 10000000;
constexpr float SMOOTHING_ALPHA = 0.3f;
}

bool Bmi270Sensor::begin() {
    sensorConnected = false;

    Serial.println("\n========================================");
    Serial.println("[BMI270] Initializing Motion Sensor via high-speed SPI...");

    pinMode(PIN_BMI270_CS, OUTPUT);
    digitalWrite(PIN_BMI270_CS, HIGH);
    delay(10);

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_BMI270_CS);
    delay(5);

    int8_t status = bmi.beginSPI(PIN_BMI270_CS, SPI_CLOCK_FREQUENCY, SPI);
    if (status != BMI2_OK) {
        Serial.printf("[BMI270] 10 MHz returned %d, attempting 4 MHz fallback...\n", status);
        status = bmi.beginSPI(PIN_BMI270_CS, 4000000, SPI);
    }

    if (status != BMI2_OK) {
        Serial.printf("[BMI270 ERROR] Init failed (error code: %d)\n", status);
        return false;
    }

    bmi2_sens_config gyroConfig{};
    gyroConfig.type = BMI2_GYRO;
    status = bmi.getConfig(&gyroConfig);
    if (status == BMI2_OK && gyroConfig.cfg.gyr.range != BMI2_GYR_RANGE_2000) {
        gyroConfig.cfg.gyr.range = BMI2_GYR_RANGE_2000;
        status = bmi.setConfig(gyroConfig);
    }
    if (status != BMI2_OK) {
        Serial.printf("[BMI270 ERROR] Unable to configure gyroscope range (error code: %d)\n", status);
        return false;
    }

    sensorConnected = true;
    Serial.println("[BMI270] Connected; gyroscope range +/-2000 deg/s");
    calibrateGyro();
    captureReferenceOrientation();
    configureMotionInterrupt();
    Serial.println("[BMI270] Calibration & interrupt setup done.\n========================================\n");
    return true;
}

bool Bmi270Sensor::update() {
    if (!sensorConnected) return false;

    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;

    if (bmi.getSensorData() != BMI2_OK) return false;
    {
        gyroX = bmi.data.gyroX * DEG_TO_RAD - biasX;
        gyroY = bmi.data.gyroY * DEG_TO_RAD - biasY;
        gyroZ = bmi.data.gyroZ * DEG_TO_RAD - biasZ;
        currentSample.gyroX = gyroX * RAD_TO_DEG;
        currentSample.gyroY = gyroY * RAD_TO_DEG;
        currentSample.gyroZ = gyroZ * RAD_TO_DEG;

        currentSample.accelX = bmi.data.accelX;
        currentSample.accelY = bmi.data.accelY;
        currentSample.accelZ = bmi.data.accelZ;

        float roll = atan2f(currentSample.accelY, currentSample.accelZ) * RAD_TO_DEG;
        float pitch = atan2f(-currentSample.accelX,
                             sqrtf(currentSample.accelY * currentSample.accelY +
                                   currentSample.accelZ * currentSample.accelZ)) * RAD_TO_DEG;
        smoothRoll = (1.0f - SMOOTHING_ALPHA) * smoothRoll + SMOOTHING_ALPHA * roll;
        smoothPitch = (1.0f - SMOOTHING_ALPHA) * smoothPitch + SMOOTHING_ALPHA * pitch;
        currentSample.roll = smoothRoll - rollReference;
        currentSample.pitch = smoothPitch - pitchReference;
    }

    float gyroMagnitude = sqrtf(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
    smoothGyro = (1.0f - SMOOTHING_ALPHA) * smoothGyro + SMOOTHING_ALPHA * gyroMagnitude;
    currentSample.angularSpeed = smoothGyro;
    currentSample.angularVelocityZ = gyroZ;
    return true;
}

bool Bmi270Sensor::read(MotionSample& sample) {
    if (!update()) return false;
    sample.acceleration = {currentSample.accelX, currentSample.accelY, currentSample.accelZ};
    sample.gyroscope = {currentSample.gyroX, currentSample.gyroY, currentSample.gyroZ};
    sample.timestampMs = millis();
    return true;
}

bool Bmi270Sensor::connected() const {
    return sensorConnected;
}

const Bmi270AudioSample& Bmi270Sensor::audioSample() const {
    return currentSample;
}

void Bmi270Sensor::enableDataReadyInterrupt(bool enable) {
    if (!sensorConnected) return;

    if (enable) {
        bmi.mapInterruptToPin(BMI2_DRDY_INT, BMI2_INT1);
        Serial.println("[BMI270] INT1 set to continuous DATA READY interrupt (100-200 Hz)");
    } else {
        bmi.mapInterruptToPin(BMI2_ANY_MOTION_INT, BMI2_INT1);
        Serial.println("[BMI270] INT1 set to ANY MOTION interrupt (movement/tap trigger)");
    }
}

void Bmi270Sensor::calibrateGyro() {
    Serial.println("[BMI270] Calibrating gyro... keep device still");

    constexpr int SAMPLE_COUNT = 200;
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        bmi.getSensorData();
        sumX += bmi.data.gyroX * DEG_TO_RAD;
        sumY += bmi.data.gyroY * DEG_TO_RAD;
        sumZ += bmi.data.gyroZ * DEG_TO_RAD;
        delay(5);
    }

    biasX = sumX / SAMPLE_COUNT;
    biasY = sumY / SAMPLE_COUNT;
    biasZ = sumZ / SAMPLE_COUNT;
}

void Bmi270Sensor::captureReferenceOrientation() {
    float axSum = 0.0f;
    float aySum = 0.0f;
    float azSum = 0.0f;

    for (int i = 0; i < 50; i++) {
        bmi.getSensorData();
        axSum += bmi.data.accelX;
        aySum += bmi.data.accelY;
        azSum += bmi.data.accelZ;
        delay(5);
    }

    float accelX = axSum / 50.0f;
    float accelY = aySum / 50.0f;
    float accelZ = azSum / 50.0f;
    rollReference = atan2f(accelY, accelZ) * RAD_TO_DEG;
    pitchReference = atan2f(-accelX, sqrtf(accelY * accelY + accelZ * accelZ)) * RAD_TO_DEG;
    smoothRoll = rollReference;
    smoothPitch = pitchReference;
    Serial.printf("[BMI270] Tilt 0-point: roll=%.1f pitch=%.1f deg\n", rollReference, pitchReference);
}

void Bmi270Sensor::configureMotionInterrupt() {
    bmi2_int_pin_config pinConfig;
    memset(&pinConfig, 0, sizeof(pinConfig));
    pinConfig.pin_type = BMI2_INT1;
    pinConfig.int_latch = BMI2_INT_NON_LATCH;
    pinConfig.pin_cfg[0].lvl = BMI2_INT_ACTIVE_HIGH;
    pinConfig.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    pinConfig.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    pinConfig.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;

    int8_t pinError = bmi.setInterruptPinConfig(pinConfig);
    int8_t featureError = bmi.enableFeature(BMI2_ANY_MOTION);
    int8_t mapError = bmi.mapInterruptToPin(BMI2_ANY_MOTION_INT, BMI2_INT1);
    Serial.printf("[BMI270] INT1 Configured (Pin: %d, Feature: %d, Map: %d)\n",
                  pinError, featureError, mapError);
}