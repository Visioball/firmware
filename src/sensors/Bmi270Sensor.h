#pragma once
#include <Arduino.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include "interfaces/IMotionSensor.h"

struct Bmi270AudioSample {
    float angularSpeed = 0.0f;
    float angularVelocityZ = 0.0f;
    float roll = 0.0f;
    float pitch = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;
    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 0.0f;
};

class Bmi270Sensor : public IMotionSensor {
public:
    bool begin() override;
    bool read(MotionSample& sample) override;
    bool connected() const;
    const Bmi270AudioSample& audioSample() const;
    void enableDataReadyInterrupt(bool enable);

private:
    bool update();
    void calibrateGyro();
    void captureReferenceOrientation();
    void configureMotionInterrupt();

    BMI270 bmi;
    Bmi270AudioSample currentSample;
    bool sensorConnected = false;
    float biasX = 0.0f;
    float biasY = 0.0f;
    float biasZ = 0.0f;
    float smoothGyro = 0.0f;
    float smoothRoll = 0.0f;
    float smoothPitch = 0.0f;
    float rollReference = 0.0f;
    float pitchReference = 0.0f;
};