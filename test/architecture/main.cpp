#include <array>
#include <cassert>

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

int main() {
  Motion motion;
  Spatial spatial(motion);
  Ball ball(motion);
  FakeLightOutput output;
  Light light(output, spatial, 45);
  Color red;
  red.red = 255;
  red.white = 42;
  Solid solid(red);

  assert(!motion.begin());
  assert(!motion.update());
  Quaternion orientation;
  assert(!motion.getOrientation(orientation));
  Vector3 position;
  Vector3 world;
  assert(!spatial.getWorldPosition(position, world));

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

  assert(ball.update(light));
  assert(output.showCount == 1);
  light.setEnabled(true);
  assert(!ball.update(light));
  assert(output.showCount == 1);
  light.setEffect(solid);
  light.setLevel(15);
  assert(ball.update(light));
  assert(output.showCount == 2);
  assert(output.brightness == 15);
  for (size_t i = 0; i < output.pixels.size(); ++i) {
    assert(output.pixels[i].red == (i < 45 ? 255 : 0));
    assert(output.pixels[i].white == (i < 45 ? 42 : 0));
  }

  HorizontalSplit split(red, Color{});
  light.setEffect(split);
  assert(!ball.update(light));
  assert(output.showCount == 2);
  assert(light.setPosition(0, position));
  assert(!light.setPosition(72, position));
  assert(!ball.update(light));
  assert(output.showCount == 2);

  light.setEnabled(false);
  assert(ball.update(light));
  assert(output.showCount == 3);
  for (const Color &pixel : output.pixels) {
    assert(pixel.red == 0 && pixel.white == 0);
  }

  Light allLeds(output, spatial, 72);
  allLeds.setEffect(solid);
  allLeds.setEnabled(true);
  assert(ball.update(allLeds));
  assert(output.pixels[71].red == 255);
  Light tooMany(output, spatial, 73);
  assert(!ball.update(tooMany));

  Sound sound(spatial);
  assert(ball.update(sound));
  sound.setEnabled(true);
  assert(!ball.update(sound));
  assert(sound.setPosition(5, position));
  assert(!sound.setPosition(6, position));
  CountingSoundEffect counting;
  sound.setEffect(counting);
  assert(ball.update(sound));
  assert(counting.count == 6);

  Beep beep;
  Synth synth;
  SoundFont soundFont;
  Spotify spotify;
  ISoundEffect *unimplemented[] = {&beep, &synth, &soundFont, &spotify};
  for (ISoundEffect *effect : unimplemented) {
    sound.setEffect(*effect);
    assert(!ball.update(sound));
  }
}
