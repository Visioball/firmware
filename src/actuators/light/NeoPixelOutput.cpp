#include "NeoPixelOutput.h"

NeoPixelOutput::NeoPixelOutput(Adafruit_NeoPixel &strip) : strip_(strip) {}

void NeoPixelOutput::clear() { strip_.clear(); }

void NeoPixelOutput::setBrightness(uint8_t brightness) {
  strip_.setBrightness(brightness);
}

void NeoPixelOutput::setPixel(size_t index, const Color &color) {
  strip_.setPixelColor(static_cast<uint16_t>(index),
                       strip_.Color(color.red, color.green, color.blue,
                                    color.white));
}

void NeoPixelOutput::show() { strip_.show(); }
