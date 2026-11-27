#include "WavDSP.h"
#include <Arduino.h>
#include "../../configuration/config.h"

int WavDSP::scanFiles() {
    trackCount = 0;
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("[WAV] Cannot scan LittleFS root");
        return 0;
    }
    File file = root.openNextFile();
    while (file && trackCount < MAX_TRACKS) {
        String filename = file.name();
        if (!file.isDirectory() && (filename.endsWith(".wav") || filename.endsWith(".WAV"))) {
            String fullPath = filename.startsWith("/") ? filename : "/" + filename;
            String title = filename.startsWith("/") ? filename.substring(1) : filename;
            title.remove(title.length() - 4);
            if (fullPath.length() >= sizeof(trackPaths[0])) {
                Serial.printf("[WAV] Filename too long: %s\n", filename.c_str());
            } else {
                strcpy(trackPaths[trackCount], fullPath.c_str());
                strcpy(trackNames[trackCount], title.c_str());
                ++trackCount;
            }
        }
        file = root.openNextFile();
    }
    for (int i = 0; i < trackCount - 1; ++i) {
        for (int j = i + 1; j < trackCount; ++j) {
            if (strcmp(trackNames[i], trackNames[j]) > 0) {
                char path[64], name[64];
                strcpy(path, trackPaths[i]);
                strcpy(name, trackNames[i]);
                strcpy(trackPaths[i], trackPaths[j]);
                strcpy(trackNames[i], trackNames[j]);
                strcpy(trackPaths[j], path);
                strcpy(trackNames[j], name);
            }
        }
    }
    return trackCount;
}

const char* WavDSP::name(int index) const {
    return index >= 0 && index < trackCount ? trackNames[index] : "";
}

const char* WavDSP::path(int index) const {
    return index >= 0 && index < trackCount ? trackPaths[index] : "";
}

bool WavDSP::initialize(const char* wavPath, float outputRate) {
    if (!source.loadFromFile(wavPath, true)) return false;

    float rate = (float)source.getSampleRate();
    voice.init(&source, outputRate > 0.0f ? outputRate : rate);
    voice.setEnvelope(0.005f, 0.05f, 1.0f, 0.2f);
    voice.noteOn(1.0f, 12000.0f);
    return true;
}

void WavDSP::start() {
    voice.noteOn(1.0f, 12000.0f);
}

void WavDSP::setRollingSpeed(float radiansPerSecond) {
    const float maxSpeed = Config::Sound::WAV_MAX_ROLLING_SPEED;
    const float normalized = constrain(
        (radiansPerSecond - Config::Motion::STATIONARY_THRESHOLD) /
            (maxSpeed - Config::Motion::STATIONARY_THRESHOLD),
        0.0f, 1.0f);
    playbackSpeed = 1.0f + normalized;
}

void WavDSP::render(float* output, int sampleCount, float, float volume) {
    voice.setPitch(1.0f);
    voice.setTempo(playbackSpeed);
    voice.setCutoff(16000.0f);
    voice.render(output, sampleCount, 0.9f);
    for (int i = 0; i < sampleCount; i++) {
        output[i] = constrain(output[i] * volume, -1.0f, 1.0f);
    }
}

float WavDSP::sampleRate() const {
    return (float)source.getSampleRate();
}

const char* WavDSP::storageType() const {
    return source.isLoadedInPsram() ? "PSRAM" : (source.isFlashMapped() ? "FLASH-MAPPED" : "NONE");
}

const char* WavDSP::loadLog() const {
    return source.getLoadLog();
}