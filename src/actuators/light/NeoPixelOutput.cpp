#include "NeoPixelOutput.h"

NeoPixelOutput::NeoPixelOutput(Adafruit_NeoPixel &strip) : strip_(strip) {}

NeoPixelOutput::NeoPixelOutput(Adafruit_NeoPixel &firstStrip,
                               Adafruit_NeoPixel &secondStrip,
                               size_t firstStripLedCount)
    : strip_(firstStrip), secondStrip_(&secondStrip),
      firstStripLedCount_(firstStripLedCount) {}

void NeoPixelOutput::clear() {
  strip_.clear();
  if (secondStrip_) secondStrip_->clear();
}

void NeoPixelOutput::setBrightness(uint8_t brightness) {
  strip_.setBrightness(brightness);
  if (secondStrip_) secondStrip_->setBrightness(brightness);
}

void NeoPixelOutput::setPixel(size_t index, const Color &color) {
  const bool useSecondStrip = secondStrip_ && index >= firstStripLedCount_;
  Adafruit_NeoPixel &target = useSecondStrip ? *secondStrip_ : strip_;
  const size_t targetIndex =
      useSecondStrip ? index - firstStripLedCount_ : index;
  target.setPixelColor(
      static_cast<uint16_t>(targetIndex),
      target.Color(color.red, color.green, color.blue, color.white));
}

void NeoPixelOutput::show() {
  strip_.show();
  if (secondStrip_) secondStrip_->show();
}
