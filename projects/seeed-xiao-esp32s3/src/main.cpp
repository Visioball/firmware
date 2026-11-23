#include <Arduino.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "Ball.h"
#include "sound/I2sOutput.h"

namespace {
Motion motion;
Spatial spatial(motion);
Light light(spatial);
I2sOutput output;
Sound sound(output);
volatile uint32_t hardwareIsrCounter = 0;
DebugLogger debug(sound, motion, hardwareIsrCounter);
Ball ball(sound, light, motion, debug);

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
        ball.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
}

void setup() {
    Serial.begin(115200);
    debug.printStartupBanner(static_cast<int>(esp_reset_reason()));
    if (!ball.begin()) {
        Serial.println("[SYSTEM] Fatal startup error; tasks were not started");
        while (true) delay(1000);
    }
    const BaseType_t soundStarted = xTaskCreatePinnedToCore(runSoundTask, "SoundDspTask", 16384,
                                     nullptr, configMAX_PRIORITIES - 1, nullptr, 1);
    const BaseType_t systemStarted = xTaskCreatePinnedToCore(runSystemTask, "SystemCoreTask", 8192,
                                      nullptr, 2, nullptr, 0);
    if (soundStarted != pdPASS || systemStarted != pdPASS) {
        Serial.println("[SYSTEM] Fatal: cannot create firmware tasks");
        esp_restart();
    }
    pinMode(PIN_BMI270_INT1, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(PIN_BMI270_INT1), onMotionInterrupt, RISING);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
