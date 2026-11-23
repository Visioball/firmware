#include <Arduino.h>
#include <math.h>
#include <Wire.h>

#include "Ball.h"
#include "actuators/light/effects/HorizontalSplit.h"
#include "actuators/light/effects/SingleLED.h"
#include "hardware/MPU9250Sensor.h"
#include "hardware/NeoPixelOutput.h"
#include "hardware/TestLedPositions.h"

namespace {

constexpr int SDA_PIN = 16;
constexpr int SCL_PIN = 17;
constexpr uint8_t LED_PIN = 18;
constexpr uint8_t IMU_ADDRESS = 0x68;
constexpr uint32_t SAMPLE_INTERVAL_MS = 10;
constexpr uint32_t PRINT_INTERVAL_MS = 250;
constexpr uint32_t EFFECT_INTERVAL_MS = 10000;
constexpr uint8_t LED_BRIGHTNESS = 96;

Adafruit_NeoPixel strip(TEST_LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
NeoPixelOutput lightOutput(strip);

MPU9250Sensor sensor(Wire, IMU_ADDRESS);
Orientation orientationEstimate;
Spatial spatial(orientationEstimate);

Motion motion(sensor, orientationEstimate);

Light light(lightOutput, spatial, TEST_LED_COUNT);
Sound sound(spatial);
Ball ball(motion, light, sound);

const Color lightColor{255, 0, 0, 0};
const Color offColor{};
HorizontalSplit horizontalSplit(lightColor, offColor);
SingleLED singleLED(TEST_LED_POSITIONS, TEST_LED_COUNT, lightColor);
bool showUpperHalf = true;
uint32_t lastSampleTime = 0;
uint32_t lastPrintTime = 0;
uint32_t lastEffectTime = 0;

} // namespace

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!ball.begin()) {
    Serial.println("MPU9250 initialization failed at address 0x68.");
    while (true) {
      delay(1000);
    }
  }

  for (size_t i = 0; i < TEST_LED_COUNT; ++i) {
    if (!light.setPosition(i, TEST_LED_POSITIONS[i])) {
      Serial.println("LED position configuration failed.");
      while (true) {
        delay(1000);
      }
    }
  }
  light.setEffect(horizontalSplit);
  light.setLevel(LED_BRIGHTNESS);
  light.setEnabled(true);

  Serial.println("MPU9250 ready.");
  Serial.println("Light effect: HorizontalSplit.");
  lastSampleTime = millis();
  lastPrintTime = lastSampleTime;
  lastEffectTime = lastSampleTime;
}


//ball.update sets a new state, updating motion, light, and sound actuators.
//Following code only reads and prints the motion sensor data.
void loop() {
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastSampleTime) < SAMPLE_INTERVAL_MS) {
    delay(1);
    return;
  }
  lastSampleTime = now;

  if (static_cast<uint32_t>(now - lastEffectTime) >= EFFECT_INTERVAL_MS) {
    lastEffectTime = now;
    showUpperHalf = !showUpperHalf;
    if (showUpperHalf) {
      light.setEffect(horizontalSplit);
      Serial.println("Light effect: HorizontalSplit.");
    } else {
      light.setEffect(singleLED);
      Serial.println("Light effect: SingleLED.");
    }
  }

  if (!ball.update()) {
    Serial.println("Ball update failed.");
    strip.clear();
    strip.show();
    delay(PRINT_INTERVAL_MS);
    return;
  }

  if (static_cast<uint32_t>(now - lastPrintTime) < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrintTime = now;
  const Motion &motion = ball.motion();
  const Vector3 &acceleration = motion.getAccelerationValue();
  const Vector3 &gyroscope = motion.getGyroscopeValue();
  const float magnitude = sqrtf(acceleration.x * acceleration.x +
                                acceleration.y * acceleration.y +
                                acceleration.z * acceleration.z);

  Serial.printf("Accel (g): X % .3f Y % .3f Z % .3f | "
                "Gyro (deg/s): X % .2f Y % .2f Z % .2f",
                acceleration.x, acceleration.y, acceleration.z,
                gyroscope.x, gyroscope.y, gyroscope.z);
  if (magnitude > 0.0f) {
    Serial.printf(" | Accel direction: X % .3f Y % .3f Z % .3f",
                  acceleration.x / magnitude, acceleration.y / magnitude,
                  acceleration.z / magnitude);
  } else {
    Serial.print(" | Accel direction unavailable");
  }
  Quaternion orientation;
  if (orientationEstimate.getOrientation(orientation)) {
    Serial.printf(" | Orientation (w,x,y,z): %.3f %.3f %.3f %.3f",
                  orientation.w, orientation.x, orientation.y, orientation.z);
  } else {
    Serial.print(" | Orientation unavailable");
  }
  Serial.println();
}
