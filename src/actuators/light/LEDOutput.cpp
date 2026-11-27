#include "LEDOutput.h"

LEDOutput::LEDOutput(Adafruit_NeoPixel &strip) : strip_(strip) {}

LEDOutput::LEDOutput(Adafruit_NeoPixel &firstStrip,
                     Adafruit_NeoPixel &secondStrip,
                     size_t secondStripStartIndex)
    : strip_(firstStrip), secondStrip_(&secondStrip),
      secondStripStartIndex_(secondStripStartIndex) {}

void LEDOutput::clear() {
  strip_.clear();
  if (secondStrip_) secondStrip_->clear();
}

void LEDOutput::setBrightness(uint8_t brightness) {
  strip_.setBrightness(brightness);
  if (secondStrip_) secondStrip_->setBrightness(brightness);
}

void LEDOutput::setPixel(size_t index, const Color &color) {
  strip_.setPixelColor(
      static_cast<uint16_t>(index),
      strip_.Color(color.red, color.green, color.blue, color.white));
  if (secondStrip_ && index >= secondStripStartIndex_) {
    secondStrip_->setPixelColor(
        static_cast<uint16_t>(index - secondStripStartIndex_),
        secondStrip_->Color(color.red, color.green, color.blue, color.white));
  }
}

void LEDOutput::show() {
  strip_.show();
  if (secondStrip_) secondStrip_->show();
}
