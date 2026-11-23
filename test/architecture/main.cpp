#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "Ball.h"
#include "actuators/light/Light.h"
#include "actuators/light/LED.h"
#include "actuators/light/effects/LavaGlow.h"
#include "actuators/light/effects/HorizontalSplit.h"
#include "actuators/light/effects/Solid.h"
#include "actuators/sound/Sound.h"
#include "actuators/sound/effects/Beep.h"
#include "actuators/sound/effects/SoundFont.h"
#include "actuators/sound/effects/Spotify.h"
#include "actuators/sound/effects/Synth.h"

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

class CountingSoundEffect : public ISoundEffect {
public:
  bool update(Speaker &speaker, const Spatial &) const override {
    assert(speaker.id == count);
    ++count;
    return true;
  }
  mutable size_t count = 0;
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

int main() {
  FakeMotionSensor sensor;
  Orientation orientationEstimate;
  Motion motion(sensor, orientationEstimate);
  Spatial spatial(orientationEstimate);
  FakeLightOutput output;
  Light light(output, spatial, 45);
  Sound sound(spatial);
  Ball ball(motion, light, sound);
  Color red;
  red.red = 255;
  red.white = 42;
  Solid solid(red);

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
  assert(output.brightness == 15);
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
  sound.setEnabled(true);
  assert(!sound.update());
  assert(sound.setPosition(5, position));
  assert(!sound.setPosition(6, position));
  CountingSoundEffect counting;
  sound.setEffect(counting);
  assert(sound.update());
  assert(counting.count == 6);

  Beep beep;
  Synth synth;
  SoundFont soundFont;
  Spotify spotify;
  ISoundEffect *unimplemented[] = {&beep, &synth, &soundFont, &spotify};
  for (ISoundEffect *effect : unimplemented) {
    sound.setEffect(*effect);
    assert(!sound.update());
  }

  counting.count = 0;
  sound.setEffect(counting);
  light.setEffect(solid);
  light.setEnabled(true);
  const int previousShows = output.showCount;
  assert(ball.update());
  assert(output.showCount == previousShows + 1);
  assert(counting.count == 6);

  counting.count = 0;
  light.setEffect(split);
  assert(!ball.update());
  assert(counting.count == 6);

  sensor.readSucceeds = false;
  counting.count = 0;
  const int showsBeforeFailedRead = output.showCount;
  assert(!ball.update());
  assert(output.showCount == showsBeforeFailedRead);
  assert(counting.count == 0);
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
}
