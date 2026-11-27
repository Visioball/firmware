#include "Sound.h"
#include <esp_timer.h>
#include <cmath>
#include <cstring>

Sound::Sound(ISoundOutput& output)
    : output(output), currentBlockSize(Config::Sound::DEFAULT_BLOCK_SIZE),
      blockBuffer(nullptr), currentMode(SoundEffectType::Synth),
      masterVolume(Config::Sound::DEFAULT_VOLUME / 100.0f), pendingTrigger(false),
      currentInterruptTimestamp(0), currentAudioStartTimestamp(0),
      audioTaskHandle(nullptr), running(false) {
    statsMux = portMUX_INITIALIZER_UNLOCKED;
    memset(&stats, 0, sizeof(stats));
}

Sound::~Sound() {
    running = false;
    free(blockBuffer);
    if (effectMutex) vSemaphoreDelete(effectMutex);
}

bool Sound::init(const char* wavPath, int blockSize) {
    currentBlockSize = blockSize == 256 ? 256 : 128;
    effectMutex = xSemaphoreCreateMutex();
    blockBuffer = static_cast<float*>(calloc(256, sizeof(float)));
    if (!effectMutex || !blockBuffer) {
        Serial.println("[SOUND] Unable to allocate renderer resources");
        return false;
    }
    if (!wavDsp.initialize(wavPath)) {
        Serial.println("[SOUND] Unable to load WAV source");
        return false;
    }
    wavLoaded = true;
    wavDsp.scanFiles();
    for (int i = 0; i < wavDsp.count(); ++i) {
        if (strcmp(wavDsp.path(i), wavPath) == 0) selectedWav = i;
    }
    stats.sampleRate = wavDsp.sampleRate();
    stats.blockSize = currentBlockSize;
    stats.idealBlockTimeUs = static_cast<int64_t>(currentBlockSize * 1000000.0f / stats.sampleRate);
    if (!output.begin(static_cast<uint32_t>(stats.sampleRate), currentBlockSize)) return false;
    streamStartUs = esp_timer_get_time();
    running = true;
    return true;
}

int Sound::scanSoundFonts() { return soundFont.scanFiles(); }
int Sound::getSoundFontCount() const { return soundFont.count(); }
const char* Sound::getSoundFontName(int index) const { return soundFont.name(index); }

bool Sound::selectSoundFontIndex(int index) {
    if (index < 0 || index >= getSoundFontCount()) return false;
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    activeEffect = nullptr;
    currentMode = SoundEffectType::Silent;
    selectedEffectIndex = -1;
    wavDsp.unload();
    wavLoaded = false;
    const bool selected = soundFont.select(index);
    soundFontLoaded = selected;
    if (selected) selectedSoundFont = index;
    else Serial.printf("[SOUND] Failed to load SoundFont: %s\n", soundFont.name(index));
    xSemaphoreGive(effectMutex);
    return selected;
}

int Sound::getEffectCount() const { return getSoundFontCount() + wavDsp.count() + 1; }

const char* Sound::getEffectName(int index) const {
    const int count = getSoundFontCount();
    if (index < 0 || index >= getEffectCount()) return "";
    if (index < count) return soundFont.name(index);
    if (index < count + wavDsp.count()) return wavDsp.name(index - count);
    return "Synth";
}

SoundEffectType Sound::getEffectType(int index) const {
    if (index < 0 || index >= getEffectCount()) return SoundEffectType::Silent;
    if (index < getSoundFontCount()) return SoundEffectType::SoundFont;
    if (index < getSoundFontCount() + wavDsp.count()) return SoundEffectType::WavDSP;
    return SoundEffectType::Synth;
}

bool Sound::selectEffectIndex(int index) {
    const int count = getSoundFontCount();
    if (index < 0 || index >= getEffectCount()) return false;
    if (index < count) {
        if (!selectSoundFontIndex(index)) return false;
        setEffectType(SoundEffectType::SoundFont);
    } else if (index < count + wavDsp.count()) {
        xSemaphoreTake(effectMutex, portMAX_DELAY);
        activeEffect = nullptr;
        currentMode = SoundEffectType::Silent;
        selectedEffectIndex = -1;
        soundFont.clear();
        soundFontLoaded = false;
        selectedWav = index - count;
        wavLoaded = wavDsp.initialize(wavDsp.path(selectedWav), stats.sampleRate);
        if (!wavLoaded) {
            Serial.printf("[SOUND] Failed to load WAV: %s\n", wavDsp.path(selectedWav));
        }
        xSemaphoreGive(effectMutex);
        if (!wavLoaded) return false;
        setEffectType(SoundEffectType::WavDSP);
    } else {
        setEffectType(SoundEffectType::Synth);
    }
    return true;
}

void Sound::setBlockSize(int size) {
    if (size != 128 && size != 256) {
        Serial.println("[SOUND] Block size must be 128 or 256");
        return;
    }
    taskENTER_CRITICAL(&statsMux);
    currentBlockSize = size;
    stats.blockSize = size;
    stats.idealBlockTimeUs = static_cast<int64_t>(size * 1000000.0f / stats.sampleRate);
    taskEXIT_CRITICAL(&statsMux);
}

void Sound::triggerMotionInterrupt(int64_t timestamp) {
    taskENTER_CRITICAL_ISR(&statsMux);
    currentInterruptTimestamp = timestamp;
    pendingTrigger = true;
    taskEXIT_CRITICAL_ISR(&statsMux);
    if (audioTaskHandle) vTaskNotifyGiveFromISR(audioTaskHandle, nullptr);
}

void Sound::updateMotion(float movement, float roll, float pitch, float rollingSpeed) {
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    wavDsp.setRollingSpeed(rollingSpeed);
    if (activeEffect) activeEffect->update(movement, roll, pitch);
    xSemaphoreGive(effectMutex);
}

void Sound::resetRollingSpeed() {
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    wavDsp.setRollingSpeed(0.0f);
    xSemaphoreGive(effectMutex);
}

void Sound::triggerHeartbeat(float intensity) {
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    if (activeEffect) activeEffect->triggerHeartbeat(intensity);
    xSemaphoreGive(effectMutex);
}

bool Sound::update() {
    return true;
}

void Sound::setMasterVolume(float percent) {
    if (!std::isfinite(percent)) {
        Serial.println("[SOUND] Volume must be finite");
        return;
    }
    percent = constrain(percent, 0.0f, Config::Sound::MAX_VOLUME);
    const float value = percent / 100.0f;
    if (std::fabs(value - masterVolume.load()) > 0.001f) {
        Serial.printf("[SOUND] Master volume: %.0f%% -> %.0f%%\n", getMasterVolume(), percent);
        masterVolume = value;
    }
}

float Sound::getMasterVolume() const { return masterVolume * 100.0f; }
SoundEffectType Sound::getEffectType() const { return currentMode; }
const char* Sound::getWavStorage() const { return wavDsp.storageType(); }
const char* Sound::getWavLoadLog() const { return wavDsp.loadLog(); }
int Sound::getBlockSize() const { return currentBlockSize; }

float Sound::getSynthFrequency() const {
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    const float value = synth.frequency();
    xSemaphoreGive(effectMutex);
    return value;
}

void Sound::setEffect(ISoundEffect& effect) {
    xSemaphoreTake(effectMutex, portMAX_DELAY);
    activeEffect = &effect;
    currentMode = effect.type();
    switch (effect.type()) {
    case SoundEffectType::SoundFont: selectedEffectIndex = selectedSoundFont; break;
    case SoundEffectType::WavDSP: selectedEffectIndex = getSoundFontCount() + selectedWav; break;
    case SoundEffectType::Synth: selectedEffectIndex = getEffectCount() - 1; break;
    default: selectedEffectIndex = -1; break;
    }
    effect.start();
    xSemaphoreGive(effectMutex);
    resetLatencyMax();
}

void Sound::setEffectType(SoundEffectType type) {
    switch (type) {
    case SoundEffectType::SoundFont:
        if (!soundFontLoaded && !selectSoundFontIndex(selectedSoundFont >= 0 ? selectedSoundFont : 0)) {
            Serial.println("[SOUND] SoundFont selection failed");
            break;
        }
        setEffect(soundFont);
        break;
    case SoundEffectType::WavDSP:
        if (!wavLoaded) {
            if (!selectEffectIndex(getSoundFontCount() + selectedWav)) {
                Serial.println("[SOUND] WAV selection failed");
            }
        } else {
            setEffect(wavDsp);
        }
        break;
    case SoundEffectType::Synth: setEffect(synth); break;
    case SoundEffectType::Beep: setEffect(beep); break;
    case SoundEffectType::Silent:
        xSemaphoreTake(effectMutex, portMAX_DELAY);
        activeEffect = nullptr;
        currentMode = type;
        selectedEffectIndex = -1;
        xSemaphoreGive(effectMutex);
        resetLatencyMax();
        break;
    }
}

void Sound::renderAudioBlock(float* output, int count) {
    memset(output, 0, count * sizeof(float));
    // Keep streaming silence while a control operation loads a SoundFont.
    if (xSemaphoreTake(effectMutex, 0) != pdTRUE) return;
    if (activeEffect && enabled) {
        activeEffect->render(output, count, stats.sampleRate, masterVolume);
    }
    xSemaphoreGive(effectMutex);
}

void Sound::renderTaskLoop() {
    audioTaskHandle = xTaskGetCurrentTaskHandle();
    int16_t pcm[256];
    while (running) {
        const int64_t blockStart = esp_timer_get_time();
        int64_t interruptTimestamp = 0;
        taskENTER_CRITICAL(&statsMux);
        const bool triggered = pendingTrigger.exchange(false);
        if (triggered) interruptTimestamp = currentInterruptTimestamp;
        taskEXIT_CRITICAL(&statsMux);
        if (triggered) {
            currentAudioStartTimestamp = esp_timer_get_time();
            if (xSemaphoreTake(effectMutex, 0) == pdTRUE) {
                if (activeEffect && currentMode == SoundEffectType::SoundFont) activeEffect->start();
                xSemaphoreGive(effectMutex);
            }
        }
        const int count = currentBlockSize;
        renderAudioBlock(blockBuffer, count);
        const int64_t blockEnd = esp_timer_get_time();
        const int64_t duration = blockEnd - blockStart;
        int peak = 0;
        int64_t sum = 0;
        for (int i = 0; i < count; ++i) {
            pcm[i] = static_cast<int16_t>(constrain(blockBuffer[i] * 32767.0f, -32768.0f, 32767.0f));
            const int magnitude = std::abs(static_cast<int>(pcm[i]));
            peak = max(peak, magnitude);
            sum += magnitude;
        }
        size_t bytesWritten = 0;
        if (!output.write(pcm, count, speakers, bytesWritten)) {
            running = false;
            Serial.println("[SOUND] Renderer stopped after output failure");
            return;
        }
        taskENTER_CRITICAL(&statsMux);
        if (triggered) {
            const int64_t latency = blockEnd - interruptTimestamp;
            stats.triggerCount++;
            stats.latencyTotalUs += latency;
            if (latency > stats.latencyMaxUs) stats.latencyMaxUs = latency;
            stats.lastIsrToStartUs = currentAudioStartTimestamp - interruptTimestamp;
            stats.lastStartToBufUs = blockEnd - currentAudioStartTimestamp;
            stats.lastTotalLatencyUs = latency;
        }
        stats.blockCount++;
        stats.blockDurationTotalUs += duration;
        if (duration > stats.blockDurationMaxUs) stats.blockDurationMaxUs = duration;
        if (duration > stats.idealBlockTimeUs) stats.underruns++;
        if (peak > stats.peakPcm) stats.peakPcm = min(peak, 32767);
        stats.avgPcm = static_cast<float>(sum) / count;
        stats.totalBytesWritten += bytesWritten;
        taskEXIT_CRITICAL(&statsMux);
    }
}

BenchmarkStats Sound::getStats() {
    taskENTER_CRITICAL(&statsMux);
    BenchmarkStats result = stats;
    result.streamElapsedUs = esp_timer_get_time() - streamStartUs;
    taskEXIT_CRITICAL(&statsMux);
    if (result.blockCount) {
        result.blockDurationAvgUs = result.blockDurationTotalUs / result.blockCount;
        result.cpuLoadPercent = result.blockDurationAvgUs * 100.0f / result.idealBlockTimeUs;
    }
    if (result.triggerCount) result.latencyAvgUs = result.latencyTotalUs / result.triggerCount;
    return result;
}

void Sound::resetPeriodicStats() {
    taskENTER_CRITICAL(&statsMux);
    stats.blockCount = 0;
    stats.blockDurationTotalUs = 0;
    stats.blockDurationMaxUs = 0;
    stats.peakPcm = 0;
    stats.avgPcm = 0.0f;
    stats.totalBytesWritten = 0;
    streamStartUs = esp_timer_get_time();
    taskEXIT_CRITICAL(&statsMux);
}

void Sound::resetLatencyMax() {
    taskENTER_CRITICAL(&statsMux);
    stats.latencyMaxUs = 0;
    stats.latencyTotalUs = 0;
    stats.triggerCount = 0;
    taskEXIT_CRITICAL(&statsMux);
}
