#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <math.h>
#include <MPU9250_WE.h>
#include <Wire.h>

#include "hardware/NeoPixelOutput.h"
#include "Ball.h"
#include "actuators/light/Light.h"
#include "actuators/light/effects/LavaGlow.h"
#include "actuators/sound/Sound.h"

namespace {

constexpr uint8_t LED_PIN = 18;
constexpr uint16_t LED_COUNT = 18;
constexpr uint16_t ACTIVE_LED_COUNT = 18;
constexpr int SDA_PIN = 16;
constexpr int SCL_PIN = 17;
constexpr uint32_t IMU_SAMPLE_INTERVAL_MS = 20;
constexpr uint32_t LED_UPDATE_INTERVAL_MS = 25;
constexpr uint32_t SERIAL_INTERVAL_MS = 250;
constexpr uint32_t RETRY_INTERVAL_MS = 5000;
constexpr uint32_t CALIBRATION_SAMPLE_INTERVAL_MS = 10;
constexpr uint8_t CALIBRATION_SAMPLE_COUNT = 100;
constexpr uint8_t LIGHT_OUTPUT_LEVEL = 96;
constexpr float CONTROL_MOTION_DEAD_ZONE_DPS = 1.5f;
constexpr float ACCEL_COUNTS_PER_G = 16384.0f;
constexpr float GYRO_COUNTS_PER_DPS = 131.072f;
constexpr uint8_t IMU_ADDRESSES[] = {0x68, 0x69};
constexpr uint8_t ACCEL_OUT_REGISTER = 0x3B;

struct ImuSample {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
};

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
NeoPixelOutput lightOutput(strip);
Motion motion;
Spatial spatial(motion);
Ball ball(motion);
Light light(lightOutput, spatial, ACTIVE_LED_COUNT);
Sound sound(spatial);
LavaGlow lavaGlow;
MPU9250_WE imuAt68(&Wire, IMU_ADDRESSES[0]);
MPU9250_WE imuAt69(&Wire, IMU_ADDRESSES[1]);
MPU9250_WE *const sensors[] = {&imuAt68, &imuAt69};
uint8_t imuAddress = 0;
bool wireReady = false;
float gyroBiasX = 0.0f;
float gyroBiasY = 0.0f;
float gyroBiasZ = 0.0f;
float xControlDegrees = 0.0f;
float yControlDegrees = 0.0f;
float zControlDegrees = -90.0f;
uint32_t lastSensorSampleTime = 0;
uint32_t lastLedUpdateTime = 0;
uint32_t lastSerialTime = 0;
uint32_t lastAttemptTime = 0;

bool readRegisters(uint8_t address, uint8_t firstRegister, uint8_t *data,
                   size_t length) {
  Wire.beginTransmission(address);
  const bool registerWritten = Wire.write(firstRegister) == 1;
  const uint8_t status = Wire.endTransmission(false);
  if (!registerWritten || status != 0) {
    return false;
  }
  if (Wire.requestFrom(address, static_cast<uint8_t>(length)) != length) {
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    const int value = Wire.read();
    if (value < 0) {
      return false;
    }
    data[i] = static_cast<uint8_t>(value);
  }
  return true;
}

int32_t signedValue(const uint8_t *data) {
  const uint16_t value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return value >= 0x8000 ? static_cast<int32_t>(value) - 0x10000 : value;
}

bool readImuSample(ImuSample &sample) {
  uint8_t data[14];
  if (!readRegisters(imuAddress, ACCEL_OUT_REGISTER, data, sizeof(data))) {
    return false;
  }
  sample.accelX = signedValue(&data[0]) / ACCEL_COUNTS_PER_G;
  sample.accelY = signedValue(&data[2]) / ACCEL_COUNTS_PER_G;
  sample.accelZ = signedValue(&data[4]) / ACCEL_COUNTS_PER_G;
  sample.gyroX = signedValue(&data[8]) / GYRO_COUNTS_PER_DPS;
  sample.gyroY = signedValue(&data[10]) / GYRO_COUNTS_PER_DPS;
  sample.gyroZ = signedValue(&data[12]) / GYRO_COUNTS_PER_DPS;
  return true;
}

bool startImu() {
  for (size_t i = 0; i < sizeof(IMU_ADDRESSES) / sizeof(IMU_ADDRESSES[0]); ++i) {
    uint8_t identity;
    if (!readRegisters(IMU_ADDRESSES[i], MPU9250_WE::REGISTER_WHO_AM_I,
                       &identity, 1) ||
        identity != MPU9250_WE::WHO_AM_I_CODE) {
      continue;
    }
    if (!sensors[i]->init()) {
      Serial.printf("MPU9250 op 0x%02X kon niet worden geinitialiseerd.\n",
                    IMU_ADDRESSES[i]);
      continue;
    }

    uint8_t ranges[2];
    if (!readRegisters(IMU_ADDRESSES[i], MPU9250_WE::REGISTER_GYRO_CONFIG,
                       ranges, sizeof(ranges)) ||
        (ranges[0] & 0x18) != 0 || (ranges[1] & 0x18) != 0) {
      Serial.println("MPU9250-meetbereik kon niet worden gecontroleerd.");
      continue;
    }
    imuAddress = IMU_ADDRESSES[i];
    Serial.printf("MPU9250 gevonden op 0x%02X.\n", imuAddress);
    return true;
  }
  Serial.println("Geen werkende MPU9250 op 0x68 of 0x69. Controleer bedrading en voeding.");
  return false;
}

bool calibrateGyroscope() {
  Serial.println("Houd de MPU9250 stil tijdens gyroscoopkalibratie.");
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;
  for (uint8_t i = 0; i < CALIBRATION_SAMPLE_COUNT; ++i) {
    ImuSample sample;
    if (!readImuSample(sample)) {
      Serial.println("Gyroscoopkalibratie mislukt; uitlezen van MPU9250 mislukt.");
      return false;
    }
    sumX += sample.gyroX;
    sumY += sample.gyroY;
    sumZ += sample.gyroZ;
    delay(CALIBRATION_SAMPLE_INTERVAL_MS);
  }
  gyroBiasX = sumX / CALIBRATION_SAMPLE_COUNT;
  gyroBiasY = sumY / CALIBRATION_SAMPLE_COUNT;
  gyroBiasZ = sumZ / CALIBRATION_SAMPLE_COUNT;
  Serial.printf("Gyroscoop-offset (graden/s): X %.2f Y %.2f Z %.2f\n",
                gyroBiasX, gyroBiasY, gyroBiasZ);
  return true;
}

bool connectAndCalibrateImu() {
  if (!startImu()) {
    return false;
  }
  if (!calibrateGyroscope()) {
    imuAddress = 0;
    return false;
  }
  lastSensorSampleTime = millis();
  return true;
}

void accumulateAxisControl(float &control, float angularRate, float elapsedSeconds) {
  if (fabsf(angularRate) < CONTROL_MOTION_DEAD_ZONE_DPS) {
    return;
  }
  control += angularRate * elapsedSeconds;
  if (control < -90.0f) {
    control = -90.0f;
  } else if (control > 90.0f) {
    control = 90.0f;
  }
}

void updateControls(const ImuSample &sample, float elapsedSeconds) {
  accumulateAxisControl(xControlDegrees, sample.gyroX - gyroBiasX,
                        elapsedSeconds);
  accumulateAxisControl(yControlDegrees, sample.gyroY - gyroBiasY,
                        elapsedSeconds);
  accumulateAxisControl(zControlDegrees, sample.gyroZ - gyroBiasZ,
                        elapsedSeconds);
}

void printImuValues(const ImuSample &sample) {
  Serial.printf("Accel (g) X: %7.3f Y: %7.3f Z: %7.3f | "
                "Gyro (graden/s) X: %8.2f Y: %8.2f Z: %8.2f\n",
                sample.accelX, sample.accelY, sample.accelZ,
                sample.gyroX - gyroBiasX, sample.gyroY - gyroBiasY,
                sample.gyroZ - gyroBiasZ);
}

} // namespace

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();
  light.setEffect(lavaGlow);
  light.setEnabled(true);
  light.setLevel(LIGHT_OUTPUT_LEVEL);

  wireReady = Wire.begin(SDA_PIN, SCL_PIN);
  if (!wireReady) {
    Serial.println("I2C initialiseren mislukt; controleer SDA en SCL.");
  } else {
    connectAndCalibrateImu();
  }
  const uint32_t now = millis();
  lastAttemptTime = now;
  lastSensorSampleTime = now;
  lastLedUpdateTime = now;
  lastSerialTime = now;
}

void loop() {
  const uint32_t now = millis();
  if (wireReady) {
    if (imuAddress == 0) {
      if (static_cast<uint32_t>(now - lastAttemptTime) >= RETRY_INTERVAL_MS) {
        lastAttemptTime = now;
        connectAndCalibrateImu();
      }
    } else if (static_cast<uint32_t>(now - lastSensorSampleTime) >=
               IMU_SAMPLE_INTERVAL_MS) {
      const uint32_t elapsedMs = now - lastSensorSampleTime;
      lastSensorSampleTime = now;
      ImuSample sample;
      if (!readImuSample(sample)) {
        Serial.println("MPU9250 uitlezen mislukt; verbinding wordt opnieuw geprobeerd.");
        imuAddress = 0;
        lastAttemptTime = now;
      } else {
        updateControls(sample, static_cast<float>(elapsedMs) / 1000.0f);
        if (static_cast<uint32_t>(now - lastSerialTime) >= SERIAL_INTERVAL_MS) {
          lastSerialTime = now;
          printImuValues(sample);
        }
      }
    }
  }

  if (static_cast<uint32_t>(now - lastLedUpdateTime) >= LED_UPDATE_INTERVAL_MS) {
    lastLedUpdateTime = now;
    lavaGlow.setControls(xControlDegrees, yControlDegrees, zControlDegrees, now);
    if (!light.update()) {
      Serial.println("Lichtupdate mislukt.");
    }
  }
  delay(2);
}
