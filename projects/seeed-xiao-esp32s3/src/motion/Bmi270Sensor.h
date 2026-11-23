#pragma once
#include <Arduino.h>
#include <SparkFun_BMI270_Arduino_Library.h>

struct MotionSample {
    float angularSpeed = 0.0f;
    float angularVelocityZ = 0.0f;
    float roll = 0.0f;
    float pitch = 0.0f;
    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 0.0f;
};

class Bmi270Sensor {
public:
    bool begin();
    void update();
    bool connected() const;
    const MotionSample& sample() const;
    void enableDataReadyInterrupt(bool enable);

private:
    void calibrateGyro();
    void captureReferenceOrientation();
    void configureMotionInterrupt();

    BMI270 bmi;
    MotionSample currentSample;
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