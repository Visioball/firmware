#include "Ball.h"
#include <LittleFS.h>
#include "connections/ble.h"

bool Ball::begin() {
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
    light.begin();
    bleInit("Visioball");
    motion.bind(this);
    motion.begin(Config::Motion::STATIONARY_THRESHOLD);
    return true;
}

void Ball::update() {
    applyReceivedConfiguration();
    motion.update(!bleClientConnected());
    IActuator* actuators[] = {&sound, &light};
    for (IActuator* actuator : actuators) actuator->update();
    debug.update();
}

void Ball::updateMotion(float movement, float roll, float pitch) {
    sound.updateMotion(movement, roll, pitch);
}

void Ball::triggerHeartbeat(float intensity) { sound.triggerHeartbeat(intensity); }
void Ball::onTapZ() { sound.onTapZ(); }
void Ball::onTapX(int direction) { sound.onTapX(direction); }

void Ball::applyReceivedConfiguration() {
    ConfigurationUpdate update;
    while (receiveConfiguration(update)) {
        if (update.hasAudioMode && !sound.selectEffectIndex(update.audioModeIndex)) {
            Serial.printf("[BLE] Audio mode index %d is unavailable\n", update.audioModeIndex);
        }
        if (update.hasVolume) sound.setLevel(update.volume);
        if (update.hasLightStatus) lightSettings.enabled = update.light.enabled;
        if (update.hasLightIntensity) lightSettings.intensity = update.light.intensity;
        if (update.hasLightColor) {
            lightSettings.red = update.light.red;
            lightSettings.green = update.light.green;
            lightSettings.blue = update.light.blue;
        }
        if (update.hasLightStatus || update.hasLightIntensity || update.hasLightColor) {
            light.apply(lightSettings);
        }
    }
}
