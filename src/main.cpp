#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <MPU9250_WE.h>
#include <Wire.h>

#include "hardware/NeoPixelOutput.h"
#include "Ball.h"
#include "actuators/light/Light.h"
#include "actuators/light/effects/Solid.h"
#include "actuators/sound/Sound.h"

namespace {

constexpr uint8_t LED_PIN = 18;
constexpr uint16_t LED_COUNT = 18;
constexpr uint16_t ACTIVE_LED_COUNT = 18;
constexpr int SDA_PIN = 16;
constexpr int SCL_PIN = 17;
constexpr uint32_t SAMPLE_INTERVAL_MS = 250;
constexpr uint32_t RETRY_INTERVAL_MS = 5000;
constexpr uint8_t IMU_ADDRESSES[] = {0x68, 0x69};
constexpr uint8_t ACCEL_OUT_REGISTER = 0x3B;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
NeoPixelOutput lightOutput(strip);
Motion motion;
Spatial spatial(motion);
Ball ball(motion);
Light light(lightOutput, spatial, ACTIVE_LED_COUNT);
Sound sound(spatial);
Solid solid(Color{});
MPU9250_WE imuAt68(&Wire, IMU_ADDRESSES[0]);
MPU9250_WE imuAt69(&Wire, IMU_ADDRESSES[1]);
MPU9250_WE *const sensors[] = {&imuAt68, &imuAt69};
uint8_t imuAddress = 0;
bool wireReady = false;
uint32_t lastSampleTime = 0;
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
    Serial.printf("MPU9250 gevonden op 0x%02X; metingen zijn niet gekalibreerd.\n",
                  imuAddress);
    return true;
  }
  Serial.println("Geen werkende MPU9250 op 0x68 of 0x69. Controleer bedrading en voeding.");
  return false;
}

int32_t signedValue(const uint8_t *data) {
  const uint16_t value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return value >= 0x8000 ? static_cast<int32_t>(value) - 0x10000 : value;
}

bool printImuValues() {
  uint8_t data[14];
  if (!readRegisters(imuAddress, ACCEL_OUT_REGISTER, data, sizeof(data))) {
    Serial.println("MPU9250 uitlezen mislukt; verbinding wordt opnieuw geprobeerd.");
    return false;
  }
  Serial.printf("Accel (g) X: %7.3f Y: %7.3f Z: %7.3f | "
                "Gyro (graden/s) X: %8.2f Y: %8.2f Z: %8.2f\n",
                signedValue(&data[0]) / 16384.0f,
                signedValue(&data[2]) / 16384.0f,
                signedValue(&data[4]) / 16384.0f,
                signedValue(&data[8]) / 131.072f,
                signedValue(&data[10]) / 131.072f,
                signedValue(&data[12]) / 131.072f);
  return true;
}

} // namespace

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();
  light.setEffect(solid);

  wireReady = Wire.begin(SDA_PIN, SCL_PIN);
  if (!wireReady) {
    Serial.println("I2C initialiseren mislukt; controleer SDA en SCL.");
    return;
  }
  startImu();
  lastAttemptTime = millis();
}

void loop() {
  if (!wireReady) {
    delay(1000);
    return;
  }
  const uint32_t now = millis();
  if (imuAddress == 0) {
    if (static_cast<uint32_t>(now - lastAttemptTime) >= RETRY_INTERVAL_MS) {
      lastAttemptTime = now;
      startImu();
    }
  } else if (static_cast<uint32_t>(now - lastSampleTime) >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    if (!printImuValues()) {
      imuAddress = 0;
      lastAttemptTime = now;
    }
  }
  delay(10);
}
