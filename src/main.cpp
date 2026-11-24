#include <Arduino.h>
#include <math.h>

#include "Ball.h"
#include "actuators/light/LEDPositions.h"
#include "actuators/light/LightSettings.h"
#include "actuators/light/NeoPixelOutput.h"
#include "actuators/light/effects/HorizontalSplit.h"
#include "actuators/light/effects/SingleLED.h"

#ifdef VISIOBALL_BMI270
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "actuators/sound/I2sOutput.h"
#include "actuators/sound/Sound.h"
#include "configuration/config.h"
#include "connections/ble.h"
#include "debug/DebugLogger.h"
#include "motion/TapDetector.h"
#include "sensors/Bmi270Sensor.h"
#else
#include <Wire.h>
#include "sensors/MPU9250Sensor.h"
#endif

namespace {
constexpr uint32_t EFFECT_INTERVAL_MS = 10000;

#ifdef VISIOBALL_BMI270
constexpr uint8_t LED_PIN = Config::Pins::LED_DATA;
Bmi270Sensor sensor;
#else
constexpr int SDA_PIN = 16;
constexpr int SCL_PIN = 17;
constexpr uint8_t LED_PIN = 18;
constexpr uint8_t IMU_ADDRESS = 0x68;
constexpr uint32_t SAMPLE_INTERVAL_MS = 10;
constexpr uint32_t PRINT_INTERVAL_MS = 250;
MPU9250Sensor sensor(Wire, IMU_ADDRESS);
#endif

Adafruit_NeoPixel strip(LED_POSITION_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
NeoPixelOutput lightOutput(strip);
Orientation orientationEstimate;
Spatial spatial(orientationEstimate);
Motion motion(sensor, orientationEstimate);
Light light(lightOutput, spatial, LED_POSITION_COUNT);
LightSettings lightSettings;

#ifdef VISIOBALL_BMI270
I2sOutput audioOutput;
Sound sound(audioOutput);
TapDetector tapDetector;
uint32_t lastMotionTime = 0;
uint32_t lastHeartbeatCycleTime = 0;
volatile uint32_t hardwareIsrCounter = 0;
DebugLogger debug(sound, sensor, hardwareIsrCounter);
#else
class DisabledAudio final : public IActuator {
public:
  bool update() override { return true; }
};
DisabledAudio sound;
uint32_t lastSampleTime = 0;
uint32_t lastPrintTime = 0;
#endif

Ball ball(motion, light, sound);
HorizontalSplit horizontalSplit({255, 0, 0, 0}, {});
SingleLED singleLED(LED_POSITIONS, LED_POSITION_COUNT, {255, 0, 0, 0});
bool showUpperHalf = true;
uint32_t lastEffectTime = 0;

void applyLightSettings() {
  light.setEnabled(lightSettings.enabled);
  light.setLevel(lightSettings.intensity);
  const Color color{lightSettings.red, lightSettings.green, lightSettings.blue, 0};
  horizontalSplit.setTopColor(color);
  singleLED.setColor(color);
}

void updateLightDemo(uint32_t now) {
  if (static_cast<uint32_t>(now - lastEffectTime) < EFFECT_INTERVAL_MS) return;
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

bool updateBall(uint32_t now) {
  updateLightDemo(now);
  if (ball.update()) return true;
  Serial.println("Ball update failed; clearing LEDs.");
  strip.clear();
  strip.show();
  return false;
}

#ifdef VISIOBALL_BMI270
void updateAudioMotion(const Bmi270AudioSample& sample, uint32_t now) {
  sound.updateMotion(sample.angularSpeed, sample.roll, sample.pitch);
  const TapGesture gesture = tapDetector.update(sample.accelX, sample.accelY, sample.accelZ, now);
  if (!bleClientConnected()) {
    switch (gesture) {
    case TapGesture::Z: sound.onTapZ(); break;
    case TapGesture::XPositive: sound.onTapX(1); break;
    case TapGesture::XNegative: sound.onTapX(-1); break;
    case TapGesture::None: break;
    }
  }
  if (sample.angularSpeed > Config::Motion::STATIONARY_THRESHOLD) {
    lastMotionTime = now;
  } else if (now - lastMotionTime >= 300 && now - lastHeartbeatCycleTime >= 1000) {
    lastHeartbeatCycleTime = now;
    sound.triggerHeartbeat(0.5f);
  }
}

void applyReceivedConfiguration() {
  ConfigurationUpdate update;
  while (receiveConfiguration(update)) {
    if (update.hasAudioMode && !sound.selectEffectIndex(update.audioModeIndex)) {
      Serial.printf("[BLE] Audio mode index %d is unavailable\n", update.audioModeIndex);
    }
    if (update.hasVolume) sound.setMasterVolume(update.volume);
    if (update.hasLightStatus) lightSettings.enabled = update.light.enabled;
    if (update.hasLightIntensity) lightSettings.intensity = update.light.intensity;
    if (update.hasLightColor) {
      lightSettings.red = update.light.red;
      lightSettings.green = update.light.green;
      lightSettings.blue = update.light.blue;
    }
    if (update.hasLightStatus || update.hasLightIntensity || update.hasLightColor) {
      applyLightSettings();
    }
  }
}

bool initializeAudio() {
  if (!LittleFS.begin(false)) {
    debug.printFileSystemMountFailed();
    if (!LittleFS.begin(true)) {
      debug.printFileSystemFailure();
      return false;
    }
  }
  debug.printFileSystemReady(LittleFS.usedBytes(), LittleFS.totalBytes());
  constexpr char TRACK[] = "/track.wav";
  if (!LittleFS.exists(TRACK)) debug.printMissingAudioTrack(TRACK);
  if (!sound.init(TRACK)) {
    debug.printAudioInitializationFailed();
    return false;
  }
  debug.printSoundFontStatus(sound.scanSoundFonts());
  sound.setEffectType(SoundEffectType::Synth);
  return true;
}

void IRAM_ATTR onMotionInterrupt() {
  hardwareIsrCounter = hardwareIsrCounter + 1;
  sound.triggerMotionInterrupt(esp_timer_get_time());
}

void runSoundTask(void*) {
  debug.printTaskStarted("Sound DSP", xPortGetCoreID(), uxTaskPriorityGet(nullptr));
  sound.renderTaskLoop();
  vTaskDelete(nullptr);
}

void runSystemTask(void*) {
  debug.printTaskStarted("System", xPortGetCoreID(), uxTaskPriorityGet(nullptr));
  while (true) {
    applyReceivedConfiguration();
    const uint32_t now = millis();
    if (updateBall(now)) {
      updateAudioMotion(sensor.audioSample(), now);
    }
    debug.update();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
#endif
} // namespace

void setup() {
  Serial.begin(115200);
#ifdef VISIOBALL_BMI270
  debug.printStartupBanner(static_cast<int>(esp_reset_reason()));
#else
  Wire.begin(SDA_PIN, SCL_PIN);
#endif
  strip.begin();
  strip.clear();
  strip.show();
  if (!ball.begin()) {
    Serial.println("Motion sensor initialization failed.");
    while (true) delay(1000);
  }
  for (size_t i = 0; i < LED_POSITION_COUNT; ++i) {
    if (!light.setPosition(i, LED_POSITIONS[i])) {
      Serial.println("LED position configuration failed.");
      while (true) delay(1000);
    }
  }
  light.setEffect(horizontalSplit);
  applyLightSettings();
  Serial.println("Light effect: HorizontalSplit.");
  lastEffectTime = millis();

#ifdef VISIOBALL_BMI270
  if (!initializeAudio()) {
    Serial.println("[SYSTEM] Fatal startup error; tasks were not started");
    while (true) delay(1000);
  }
  bleInit("Visioball");
  const BaseType_t soundStarted = xTaskCreatePinnedToCore(
      runSoundTask, "SoundDspTask", 16384, nullptr, configMAX_PRIORITIES - 1, nullptr, 1);
  const BaseType_t systemStarted = xTaskCreatePinnedToCore(
      runSystemTask, "SystemCoreTask", 8192, nullptr, 2, nullptr, 0);
  if (soundStarted != pdPASS || systemStarted != pdPASS) {
    Serial.println("[SYSTEM] Fatal: cannot create firmware tasks");
    esp_restart();
  }
  pinMode(PIN_BMI270_INT1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PIN_BMI270_INT1), onMotionInterrupt, RISING);
#else
  Serial.println("MPU9250 ready.");
  lastSampleTime = millis();
  lastPrintTime = lastSampleTime;
#endif
}

void loop() {
#ifdef VISIOBALL_BMI270
  vTaskDelay(pdMS_TO_TICKS(1000));
#else
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastSampleTime) < SAMPLE_INTERVAL_MS) {
    delay(1);
    return;
  }
  lastSampleTime = now;
  if (!updateBall(now)) {
    delay(PRINT_INTERVAL_MS);
    return;
  }
  if (static_cast<uint32_t>(now - lastPrintTime) < PRINT_INTERVAL_MS) return;
  lastPrintTime = now;
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
#endif
}
