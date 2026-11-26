#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "Ball.h"
#include "actuators/light/Light.h"
#include "actuators/light/LED.h"
#include "actuators/light/LEDPositions.h"
#include "actuators/light/effects/LavaGlow.h"
#include "actuators/light/effects/HorizontalSplit.h"
#include "actuators/light/effects/SingleLED.h"
#include "actuators/light/effects/Solid.h"
#include "energy/Energy.h"
#include "connections/CommunicationProtocol.h"
#include "interfaces/IActuator.h"
#include "interfaces/IMotionSensor.h"

class FakeLightOutput : public ILightOutput {
public:
  void clear() override {
    for (Color &pixel : pixels) {
      pixel = Color{};
    }
    ++clearCount;
  }
  void setBrightness(uint8_t value) override { brightness = value; }
  void setPixel(size_t index, const Color &color) override {
    assert(index < pixels.size());
    pixels[index] = color;
  }
  void show() override { ++showCount; }

  std::array<Color, 72> pixels;
  uint8_t brightness = 0;
  int clearCount = 0;
  int showCount = 0;
};

// The firmware sound engine needs ESP32 hardware; Ball only needs IActuator.
class FakeSoundActuator : public IActuator {
public:
  bool update() override {
    ++updateCount;
    return succeeds;
  }
  int updateCount = 0;
  bool succeeds = true;
};

class FakeMotionSensor : public IMotionSensor {
public:
  FakeMotionSensor() {
    nextSample.acceleration.x = 0.1f;
    nextSample.acceleration.y = 0.2f;
    nextSample.acceleration.z = 0.9f;
    nextSample.gyroscope.x = 1.0f;
    nextSample.gyroscope.y = 2.0f;
    nextSample.gyroscope.z = 3.0f;
  }
  bool begin() override { return true; }
  bool read(MotionSample &sample) override {
    if (!readSucceeds) {
      return false;
    }
    sample = nextSample;
    nextSample.timestampMs += 10;
    return true;
  }
  bool readSucceeds = true;
  MotionSample nextSample;
};

class FakeBatteryLevelMeter : public BatteryLevelMeter {
public:
  bool readLevel(uint8_t &percent) override {
    percent = level;
    return succeeds;
  }
  uint8_t level = 0;
  bool succeeds = true;
};

int main() {
  CommunicationProtocol::DeviceStatus status;
  status.lightEnabled = true;
  status.lightIntensity = 128;
  status.audioPlaying = true;
  status.audioVolume = 42;
  status.charging = true;
  auto packet = CommunicationProtocol::encode(status);
  assert((packet == std::array<uint8_t, 5>{1, 3, 128, 42, 0}));
  status.batteryAvailable = true;
  status.batteryLevel = 75;
  status.chargingAvailable = true;
  packet = CommunicationProtocol::encode(status);
  assert((packet == std::array<uint8_t, 5>{1, 31, 128, 42, 75}));

  Energy withoutBatteryMeter;
  uint8_t unavailableLevel = 42;
  assert(!withoutBatteryMeter.getBatteryLevel(unavailableLevel) && unavailableLevel == 42);
  assert(!withoutBatteryMeter.lightAllowed());
  assert(withoutBatteryMeter.updateBleConnection(true));
  assert(withoutBatteryMeter.lightAllowed());
  assert(!withoutBatteryMeter.updateBleConnection(true));
  assert(withoutBatteryMeter.updateBleConnection(false));
  assert(!withoutBatteryMeter.lightAllowed());
  assert(!withoutBatteryMeter.shouldSleep(Energy::INACTIVITY_MS));
  withoutBatteryMeter.setPowerSavingEnabled(true, 100);
  assert(!withoutBatteryMeter.shouldSleep(100 + Energy::INACTIVITY_MS - 1));
  assert(withoutBatteryMeter.shouldSleep(100 + Energy::INACTIVITY_MS));
  withoutBatteryMeter.recordActivity(100 + Energy::INACTIVITY_MS);
  assert(!withoutBatteryMeter.shouldSleep(100 + Energy::INACTIVITY_MS));
  withoutBatteryMeter.updateBleConnection(true);
  assert(!withoutBatteryMeter.shouldSleep(100 + 2 * Energy::INACTIVITY_MS));
  withoutBatteryMeter.updateBleConnection(false);
  withoutBatteryMeter.setPowerSavingEnabled(false, 200);
  assert(!withoutBatteryMeter.shouldSleep(200 + Energy::INACTIVITY_MS));
  withoutBatteryMeter.setPowerSavingEnabled(true, UINT32_MAX - 100);
  assert(!withoutBatteryMeter.shouldSleep(Energy::INACTIVITY_MS - 102));
  assert(withoutBatteryMeter.shouldSleep(Energy::INACTIVITY_MS - 101));

  FakeBatteryLevelMeter batteryMeter;
  Energy energy(batteryMeter);
  uint8_t batteryLevel = 42;
  assert(energy.getBatteryLevel(batteryLevel) && batteryLevel == 0);
  batteryMeter.level = 100;
  assert(energy.getBatteryLevel(batteryLevel) && batteryLevel == 100);
  batteryMeter.level = 101;
  assert(!energy.getBatteryLevel(batteryLevel) && batteryLevel == 100);
  batteryMeter.succeeds = false;
  batteryMeter.level = 25;
  assert(!energy.getBatteryLevel(batteryLevel) && batteryLevel == 100);

  FakeMotionSensor sensor;
  Orientation orientationEstimate;
  Motion motion(sensor, orientationEstimate);
  Spatial spatial(orientationEstimate);
  FakeLightOutput output;
  Light light(output, spatial, 45);
  FakeSoundActuator sound;
  Ball ball(motion, light, sound);
  Color red;
  red.red = 255;
  red.white = 42;
  Solid solid(red);

  FakeLightOutput limitedOutput;
  Light limitedLight(limitedOutput, spatial, 1);
  limitedLight.setEffect(solid);
  limitedLight.setEnabled(true);
  for (const auto &sample : std::array<std::array<uint8_t, 2>, 6>{{
           {{0, 0}}, {{1, 0}}, {{40, 16}}, {{128, 50}}, {{254, 100}}, {{255, 100}}
       }}) {
    limitedLight.setLevel(sample[0]);
    assert(limitedLight.update());
    assert(limitedOutput.brightness == sample[1]);
  }

  Quaternion orientation;
  Vector3 position;
  Vector3 world;
  assert(!orientationEstimate.getOrientation(orientation));
  assert(!spatial.getWorldPosition(position, world));
  assert(ball.begin());
  assert(ball.update());
  assert(ball.motion().getAccelerationValue().x == 0.1f);
  assert(ball.motion().getAccelerationValue().y == 0.2f);
  assert(ball.motion().getAccelerationValue().z == 0.9f);
  assert(ball.motion().getGyroscopeValue().x == 1.0f);
  assert(output.showCount == 1);
  output.showCount = 0;
  assert(orientationEstimate.getOrientation(orientation));
  assert(spatial.getWorldPosition(position, world));

  LavaGlow lavaGlow;
  LED lavaPixel;
  lavaGlow.setControls(0.0f, -90.0f, 0.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  const Color redGlow = lavaPixel.color;
  assert(redGlow.red > 0 && redGlow.green == 0 && redGlow.blue == 0);

  lavaGlow.setControls(0.0f, 90.0f, 0.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  assert(lavaPixel.color.red > 0 && lavaPixel.color.green > 0);

  lavaGlow.setControls(-90.0f, 0.0f, 0.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int lowIntensity =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  lavaGlow.setControls(90.0f, 0.0f, 0.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int highIntensity =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  assert(lowIntensity > 0 && highIntensity > lowIntensity);

  lavaGlow.setControls(0.0f, 0.0f, 0.0f, 0);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int firstGlow =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  lavaGlow.setControls(0.0f, 0.0f, 0.0f, 1000);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int laterGlow =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  assert(firstGlow != laterGlow);

  lavaGlow.setControls(0.0f, 0.0f, 0.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int slowGlow =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  lavaGlow.setControls(0.0f, 0.0f, 90.0f, 500);
  assert(lavaGlow.update(lavaPixel, spatial));
  const unsigned int fastGlow =
      lavaPixel.color.red + lavaPixel.color.green + lavaPixel.color.blue;
  assert(slowGlow != fastGlow);

  assert(light.update());
  assert(output.showCount == 1);
  light.setEnabled(true);
  assert(!light.update());
  assert(output.showCount == 1);
  light.setEffect(solid);
  light.setLevel(15);
  assert(light.update());
  assert(output.showCount == 2);
  assert(output.brightness == 6);
  for (size_t i = 0; i < output.pixels.size(); ++i) {
    assert(output.pixels[i].red == (i < 45 ? 255 : 0));
    assert(output.pixels[i].white == (i < 45 ? 42 : 0));
  }

  HorizontalSplit split(red, Color{});
  light.setEffect(split);
  assert(!light.update());
  assert(output.showCount == 2);
  assert(light.setPosition(0, position));
  assert(!light.setPosition(72, position));
  assert(!light.update());
  assert(output.showCount == 2);

  light.setEnabled(false);
  assert(light.update());
  assert(output.showCount == 3);
  for (const Color &pixel : output.pixels) {
    assert(pixel.red == 0 && pixel.white == 0);
  }

  Light allLeds(output, spatial, 72);
  allLeds.setEffect(solid);
  allLeds.setEnabled(true);
  assert(allLeds.update());
  assert(output.pixels[71].red == 255);
  Light tooMany(output, spatial, 73);
  assert(!tooMany.update());

  assert(sound.update());
  sound.succeeds = false;
  assert(!sound.update());
  sound.succeeds = true;

  sound.updateCount = 0;
  light.setEffect(solid);
  light.setEnabled(true);
  const int previousShows = output.showCount;
  assert(ball.update());
  assert(output.showCount == previousShows + 1);
  assert(sound.updateCount == 1);

  sound.succeeds = false;
  assert(!ball.update());
  sound.succeeds = true;

  sound.updateCount = 0;
  light.setEffect(split);
  assert(!ball.update());
  assert(sound.updateCount == 1);

  sensor.readSucceeds = false;
  sound.updateCount = 0;
  const int showsBeforeFailedRead = output.showCount;
  assert(!ball.update());
  assert(output.showCount == showsBeforeFailedRead);
  assert(sound.updateCount == 0);
  assert(!orientationEstimate.getOrientation(orientation));

  FakeMotionSensor orientationSensor;
  orientationSensor.nextSample.acceleration.x = 0.0f;
  orientationSensor.nextSample.acceleration.y = 0.0f;
  orientationSensor.nextSample.acceleration.z = 1.0f;
  orientationSensor.nextSample.gyroscope = Vector3{};
  orientationSensor.nextSample.timestampMs = UINT32_MAX - 40;
  Orientation orientationEstimate2;
  Motion orientationMotion(orientationSensor, orientationEstimate2);
  Spatial orientationSpatial(orientationEstimate2);
  assert(orientationMotion.begin());
  assert(orientationMotion.update());

  Vector3 localY;
  localY.y = 1.0f;
  orientationSensor.nextSample.acceleration.z = 2.0f;
  orientationSensor.nextSample.gyroscope.x = 90.0f;
  for (int i = 0; i < 100; ++i) {
    assert(orientationMotion.update());
  }
  assert(orientationSpatial.getWorldPosition(localY, world));
  assert(std::fabs(world.y) < 0.01f);
  assert(std::fabs(world.z - 1.0f) < 0.01f);
  assert(orientationEstimate2.getOrientation(orientation));
  const float quaternionNorm =
      std::sqrt(orientation.w * orientation.w + orientation.x * orientation.x +
                orientation.y * orientation.y + orientation.z * orientation.z);
  assert(std::fabs(quaternionNorm - 1.0f) < 0.001f);

  orientationSensor.nextSample.acceleration.z = 1.0f;
  orientationSensor.nextSample.gyroscope.x = 0.0f;
  for (int i = 0; i < 300; ++i) {
    assert(orientationMotion.update());
  }
  assert(orientationSpatial.getWorldPosition(localY, world));
  assert(std::fabs(world.z) < 0.01f);

  assert(orientationMotion.begin());
  orientationSensor.nextSample.acceleration.z = -1.0f;
  assert(orientationMotion.update());
  Vector3 localZ;
  localZ.z = 1.0f;
  assert(orientationSpatial.getWorldPosition(localZ, world));
  assert(std::fabs(world.z + 1.0f) < 0.001f);

  FakeLightOutput orientationOutput;
  Light orientationLight(orientationOutput, orientationSpatial, 2);
  HorizontalSplit orientationSplit(red, Color{});
  orientationLight.setEffect(orientationSplit);
  orientationLight.setEnabled(true);
  assert(orientationLight.setPosition(0, localZ));
  localZ.z = -1.0f;
  assert(orientationLight.setPosition(1, localZ));
  assert(orientationLight.update());
  assert(orientationOutput.pixels[0].red == 0);
  assert(orientationOutput.pixels[1].red == 255);

  assert(orientationMotion.begin());
  orientationSensor.nextSample.acceleration.z = 0.0f;
  assert(orientationMotion.update());
  assert(!orientationEstimate2.getOrientation(orientation));

  orientationSensor.nextSample.acceleration.z = 1.0f;
  assert(orientationMotion.update());
  orientationSensor.nextSample.timestampMs += 300;
  orientationSensor.nextSample.acceleration.x = 1.0f;
  orientationSensor.nextSample.acceleration.z = 0.0f;
  assert(orientationMotion.update());
  Vector3 localX;
  localX.x = 1.0f;
  assert(orientationSpatial.getWorldPosition(localX, world));
  assert(std::fabs(world.z - 1.0f) < 0.001f);

  orientationSensor.nextSample.gyroscope.x =
      std::numeric_limits<float>::quiet_NaN();
  assert(orientationMotion.update());
  assert(!orientationEstimate2.getOrientation(orientation));

  FakeMotionSensor mappedSensor;
  mappedSensor.nextSample.acceleration.x = 0.0f;
  mappedSensor.nextSample.acceleration.y = 0.0f;
  mappedSensor.nextSample.acceleration.z = 1.0f;
  mappedSensor.nextSample.gyroscope = Vector3{};
  Orientation mappedOrientation;
  Motion mappedMotion(mappedSensor, mappedOrientation);
  Spatial mappedSpatial(mappedOrientation);
  FakeLightOutput mappedOutput;
  Light mappedLight(mappedOutput, mappedSpatial, LED_POSITION_COUNT);
  SingleLED singleLED(LED_POSITIONS, LED_POSITION_COUNT, red);
  HorizontalSplit mappedSplit(red, Color{});
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedLight.setPosition(i, LED_POSITIONS[i]));
  }
  mappedLight.setEffect(singleLED);
  mappedLight.setEnabled(true);
  assert(!mappedLight.update());
  assert(mappedOutput.showCount == 0);

  assert(mappedMotion.begin());
  assert(mappedMotion.update());
  mappedLight.setEffect(mappedSplit);
  assert(mappedLight.update());
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedOutput.pixels[i].red == (i >= 9 ? 255 : 0));
  }

  mappedLight.setEffect(singleLED);
  assert(mappedLight.update());
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedOutput.pixels[i].red == (i == 9 ? 255 : 0));
  }

  HorizontalSplit fadingSplit({255, 0, 64, 200}, {0, 200, 64, 0});
  LED fadingLed;
  fadingLed.hasPosition = true;
  const float fadeHeights[] = {-1.0f, 0.0f, 0.0375f, 0.075f,
                               0.1125f, 0.15f, 1.0f};
  const uint8_t expectedRed[] = {0, 0, 40, 128, 215, 255, 255};
  const uint8_t expectedGreen[] = {200, 200, 169, 100, 31, 0, 0};
  const uint8_t expectedWhite[] = {0, 0, 31, 100, 169, 200, 200};
  for (size_t i = 0; i < 7; ++i) {
    fadingLed.position.z = fadeHeights[i];
    assert(fadingSplit.update(fadingLed, mappedSpatial));
    assert(fadingLed.color.red == expectedRed[i]);
    assert(fadingLed.color.green == expectedGreen[i]);
    assert(fadingLed.color.blue == 64);
    assert(fadingLed.color.white == expectedWhite[i]);
  }
  const float lowerHeights[] = {-1.0f, -0.15f, -0.019f, -0.001f, 0.0f};
  for (float height : lowerHeights) {
    fadingLed.position.z = height;
    assert(mappedSplit.update(fadingLed, mappedSpatial));
    assert(fadingLed.color.red == 0 && fadingLed.color.green == 0);
    assert(fadingLed.color.blue == 0 && fadingLed.color.white == 0);
  }
  fadingLed.hasPosition = false;
  assert(!fadingSplit.update(fadingLed, mappedSpatial));

  mappedSensor.nextSample.acceleration.z = 2.0f;
  mappedSensor.nextSample.gyroscope.x = 80.0f;
  for (int i = 0; i < 100; ++i) {
    assert(mappedMotion.update());
  }
  assert(mappedLight.update());
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedOutput.pixels[i].red == (i == 14 ? 255 : 0));
  }

  mappedSensor.nextSample.gyroscope.x = 0.0f;
  mappedSensor.nextSample.acceleration.x = 0.8660254f;
  mappedSensor.nextSample.acceleration.z = 0.5f;
  assert(mappedMotion.begin());
  assert(mappedMotion.update());
  assert(mappedLight.update());
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedOutput.pixels[i].red == (i == 12 ? 255 : 0));
  }

  mappedSensor.nextSample.acceleration.x = 0.0f;
  mappedSensor.nextSample.acceleration.z = -1.0f;
  assert(mappedMotion.begin());
  assert(mappedMotion.update());
  mappedLight.setEffect(mappedSplit);
  assert(mappedLight.update());
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    assert(mappedOutput.pixels[i].red == (i < 9 ? 255 : 0));
  }
}
