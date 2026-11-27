#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

class Adafruit_NeoPixel {
public:
  explicit Adafruit_NeoPixel(std::size_t count) : pixels(count) {}

  void clear() { pixels.assign(pixels.size(), 0); }
  void setBrightness(uint8_t value) { brightness = value; }
  void setPixelColor(uint16_t index, uint32_t color) {
    assert(index < pixels.size());
    pixels[index] = color;
  }
  uint32_t Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    return (uint32_t(white) << 24) | (uint32_t(red) << 16) |
           (uint32_t(green) << 8) | blue;
  }
  void show() { ++showCount; }

  std::vector<uint32_t> pixels;
  uint8_t brightness = 0;
  int showCount = 0;
};
