#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstring>

namespace {

constexpr uint8_t LED_PIN = 17;
// The strip is physically 60 pixels, but only the first 45 are lit (USB power).
// Data must still be sent to all 60 so the unused pixels are explicitly off.
constexpr uint16_t LED_COUNT = 60;
constexpr uint16_t ACTIVE_LED_COUNT = 45;
constexpr uint32_t SIMULATION_INTERVAL_MS = 3000;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

const char *const simulatedInputs[] = {
    R"json({"light":{"status":true,"intensity":5,"type":"static","color":{"r":255,"g":0,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":10,"type":"static","color":{"r":255,"g":0,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":15,"type":"static","color":{"r":255,"g":0,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":5,"type":"static","color":{"r":0,"g":255,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":10,"type":"static","color":{"r":0,"g":255,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":15,"type":"static","color":{"r":0,"g":255,"b":0,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":5,"type":"static","color":{"r":0,"g":0,"b":255,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":10,"type":"static","color":{"r":0,"g":0,"b":255,"w":0}}})json",
    R"json({"light":{"status":true,"intensity":15,"type":"static","color":{"r":0,"g":0,"b":255,"w":0}}})json",
};

size_t nextInputIndex = 0;
uint32_t lastInputTime = 0;
bool hasProcessedInput = false;

bool readColorComponent(JsonObjectConst color, const char *key, uint8_t &value) {
  const JsonVariantConst component = color[key];
  if (!component.is<int>()) {
    return false;
  }

  const int parsedValue = component.as<int>();
  if (parsedValue < 0 || parsedValue > 255) {
    return false;
  }

  value = static_cast<uint8_t>(parsedValue);
  return true;
}

bool applyLightJson(const char *json) {
  JsonDocument document;
  const DeserializationError error = deserializeJson(document, json);
  if (error) {
    Serial.print("Could not parse light JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  const JsonVariantConst lightValue = document["light"];
  if (!lightValue.is<JsonObjectConst>()) {
    Serial.println("Light JSON is missing the light object.");
    return false;
  }

  const JsonObjectConst light = lightValue.as<JsonObjectConst>();
  const JsonVariantConst statusValue = light["status"];
  const JsonVariantConst intensityValue = light["intensity"];
  const JsonVariantConst typeValue = light["type"];
  const JsonVariantConst colorValue = light["color"];

  if (!statusValue.is<bool>() || !intensityValue.is<int>() ||
      !typeValue.is<const char *>() || !colorValue.is<JsonObjectConst>()) {
    Serial.println("Light JSON has missing or invalid fields.");
    return false;
  }

  const int intensity = intensityValue.as<int>();
  const char *type = typeValue.as<const char *>();
  if (intensity < 0 || intensity > 255) {
    Serial.println("Light intensity must be between 0 and 255.");
    return false;
  }
  if (std::strcmp(type, "static") != 0) {
    Serial.print("Unsupported light type: ");
    Serial.println(type);
    return false;
  }

  const JsonObjectConst color = colorValue.as<JsonObjectConst>();
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white = 0;
  if (!readColorComponent(color, "r", red) ||
      !readColorComponent(color, "g", green) ||
      !readColorComponent(color, "b", blue)) {
    Serial.println("Light color components must be between 0 and 255.");
    return false;
  }
  // "w" is optional so RGB-only payloads keep working.
  if (!color["w"].isNull() && !readColorComponent(color, "w", white)) {
    Serial.println("Light white component must be between 0 and 255.");
    return false;
  }

  if (!statusValue.as<bool>()) {
    strip.clear();
    strip.show();
    Serial.println("Light strip off.");
    return true;
  }

  strip.setBrightness(static_cast<uint8_t>(intensity));
  strip.clear();
  strip.fill(strip.Color(red, green, blue, white), 0, ACTIVE_LED_COUNT);
  strip.show();
  Serial.println("Applied light settings.");
  return true;
}

void processSimulatedInput() {
  const size_t inputCount = sizeof(simulatedInputs) / sizeof(simulatedInputs[0]);
  applyLightJson(simulatedInputs[nextInputIndex]);
  nextInputIndex = (nextInputIndex + 1) % inputCount;
}

} // namespace

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();
}

void loop() {
  const uint32_t now = millis();
  if (!hasProcessedInput ||
      static_cast<uint32_t>(now - lastInputTime) >= SIMULATION_INTERVAL_MS) {
    processSimulatedInput();
    lastInputTime = now;
    hasProcessedInput = true;
  }
}
