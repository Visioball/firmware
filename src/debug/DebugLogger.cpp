#include "DebugLogger.h"
#include "../actuators/sound/Sound.h"
#include "../sensors/Bmi270Sensor.h"
#include <esp_heap_caps.h>
#include <esp_system.h>

void DebugLogger::printStartupBanner(int resetReason) {
    const char* reasonName = "unknown";
    switch (resetReason) {
    case ESP_RST_POWERON: reasonName = "power-on"; break;
    case ESP_RST_EXT: reasonName = "external reset"; break;
    case ESP_RST_SW: reasonName = "software reset"; break;
    case ESP_RST_PANIC: reasonName = "panic"; break;
    case ESP_RST_INT_WDT: reasonName = "interrupt watchdog"; break;
    case ESP_RST_TASK_WDT: reasonName = "task watchdog"; break;
    case ESP_RST_WDT: reasonName = "watchdog"; break;
    case ESP_RST_DEEPSLEEP: reasonName = "deep sleep"; break;
    case ESP_RST_BROWNOUT: reasonName = "brownout"; break;
    case ESP_RST_SDIO: reasonName = "SDIO reset"; break;
    }
    Serial.printf("[SYSTEM] Reset reason: %d (%s)\n", resetReason, reasonName);
    Serial.println("\n\n##################################################################");
    Serial.println("           VISIOBALL ESP32-S3 SOUND & DSP BENCHMARK               ");
    Serial.println("##################################################################");
}

void DebugLogger::printTaskStarted(const char* taskName, int core, int priority) {
    Serial.printf("[%s] running on Core %d (Priority %d)\n", taskName, core, priority);
}

void DebugLogger::printFileSystemMountFailed() {
    Serial.println("[SYSTEM] LittleFS mount failed, formatting...");
}

void DebugLogger::printFileSystemFailure() {
    Serial.println("[SYSTEM] FATAL: LittleFS unavailable!");
}

void DebugLogger::printFileSystemReady(size_t usedBytes, size_t totalBytes) {
    Serial.printf("[SYSTEM] LittleFS mounted: %u KB used / %u KB total\n",
                  (unsigned)(usedBytes / 1024), (unsigned)(totalBytes / 1024));
}

void DebugLogger::printMissingAudioTrack(const char* path) {
    Serial.printf("[SYSTEM] WARNING: %s not found! Run 'pio run -t uploadfs'\n", path);
}

void DebugLogger::printAudioInitializationFailed() {
    Serial.println("[SYSTEM] FATAL: Audio Engine initialization failed!");
}

void DebugLogger::printSoundFontStatus(int count) {
    if (count > 0) {
        Serial.println("[SYSTEM] SoundFonts ready; select via BLE or serial keys 1..9.");
    } else {
        Serial.println("[SYSTEM] WARNING: No SoundFont (.sf2) files found on LittleFS!");
    }
}

void DebugLogger::printVolumeChanged(float volumePercent) {
    Serial.printf("\n[AUDIO] Volume changed -> %.0f%%\n", volumePercent);
}

void DebugLogger::printModeChanged(const char* modeName) {
    Serial.printf("\n[AUDIO] Mode changed -> %s\n", modeName);
}

void DebugLogger::togglePrint() {
    printEnabled = !printEnabled;
    Serial.printf("[CMD] Benchmark printing: %s\n", printEnabled ? "ON" : "OFF");
}

void DebugLogger::setPrintEnabled(bool enabled) {
    printEnabled = enabled;
}

void DebugLogger::toggleAutoBlockSize() {
    autoToggleBlockSize = !autoToggleBlockSize;
    Serial.printf("[CMD] Auto toggle block size: %s\n", autoToggleBlockSize ? "ON" : "OFF");
}

void DebugLogger::handleSerialCommands() {
    if (!Serial.available()) return;

    char cmd = (char)Serial.read();

    // 1..9: Select SoundFont by index
    if (cmd >= '1' && cmd <= '9') {
        int sfIndex = cmd - '1';
        if (sfIndex < sound.getSoundFontCount()) {
            if (sound.selectSoundFontIndex(sfIndex)) {
                sound.setEffectType(SoundEffectType::SoundFont);
            } else {
                Serial.printf("[CMD] Unable to load SoundFont at index %d\n", sfIndex + 1);
            }
        } else {
            Serial.printf("[CMD] No SoundFont at index %d (available: 1..%d)\n",
                          sfIndex + 1, sound.getSoundFontCount());
        }
    } else if (cmd == 's' || cmd == 'S') {
        sound.setEffectType(SoundEffectType::SoundFont);
    } else if (cmd == 'd' || cmd == 'D') {
        sound.setEffectType(SoundEffectType::WavDSP);
    } else if (cmd == 'm' || cmd == 'M') {
        sound.setEffectType(SoundEffectType::Synth);
    } else if (cmd == 'i' || cmd == 'I') {
        dataReadyIntActive = !dataReadyIntActive;
        sensor.enableDataReadyInterrupt(dataReadyIntActive);
    } else if (cmd == 'r' || cmd == 'R') {
        sound.resetLatencyMax();
        hardwareIsrCounter = 0;
        prevReportedIsrCount = 0;
        Serial.println("[CMD] Latency Max and ISR Counter reset!");
    } else if (cmd == '+') {
        float currentVol = sound.getMasterVolume();
        sound.setMasterVolume(currentVol + 10.0f);
    } else if (cmd == '-') {
        float currentVol = sound.getMasterVolume();
        sound.setMasterVolume(currentVol - 10.0f);
    } else if (cmd == 'p' || cmd == 'P') {
        togglePrint();
    }
}

void DebugLogger::update() {
    uint32_t now = millis();

    // Optional: Auto toggle between 128 and 256 sample buffers every 10 seconds
    if (autoToggleBlockSize && (now - lastBlockSizeSwitchTime >= 10000)) {
        int nextBlockSize = (sound.getBlockSize() == 128) ? 256 : 128;
        sound.setBlockSize(nextBlockSize);
        Serial.printf("\n>>> [BENCHMARK] Switched Buffer Block Size to %d samples <<<\n\n", nextBlockSize);
        lastBlockSizeSwitchTime = now;
    }

    // Process interactive serial commands
    handleSerialCommands();

    // Periodic 1-second benchmark logging
    if (printEnabled && now - lastLogTime >= 1000) {
        BenchmarkStats stats = sound.getStats();

        uint32_t currentIsrTotal = hardwareIsrCounter;
        uint32_t isrInLastSec = currentIsrTotal - prevReportedIsrCount;
        prevReportedIsrCount = currentIsrTotal;

        size_t freeHeap = ESP.getFreeHeap() / 1024;
        size_t freePsram = ESP.getFreePsram() / (1024 * 1024);
        size_t totalPsram = ESP.getPsramSize() / (1024 * 1024);

        const char* modeStr = sound.getEffectType() == SoundEffectType::SoundFont ? "SOUNDFONT" :
                      sound.getEffectType() == SoundEffectType::WavDSP ? "WAV DSP" :
                      sound.getEffectType() == SoundEffectType::Silent ? "SILENT MODE TRANSITION" :
                      sound.getEffectType() == SoundEffectType::Beep ? "BEEP" : "SYNTH";

        Serial.println("==================================================================");
        Serial.printf(" [VISIOBAL AUDIO BENCHMARK]  Mode: %s\n", modeStr);
        Serial.printf(" Buffer: %d samples (Ideal: %lld us) | Vol: %.0f%%\n",
                      stats.blockSize, stats.idealBlockTimeUs, sound.getMasterVolume());
        Serial.printf(" Sample Rate: %.0f Hz | Peak PCM: %d | Avg PCM: %.1f | Stream: %u KB\n",
                      stats.sampleRate, stats.peakPcm, stats.avgPcm, (unsigned)(stats.totalBytesWritten / 1024));
        if (stats.streamElapsedUs > 0) {
            const float framesPerSecond =
                (static_cast<float>(stats.totalBytesWritten) / 4.0f) *
                1000000.0f / static_cast<float>(stats.streamElapsedUs);
            Serial.printf(" I2S write rate: %.0f frames/s over %.2f s (approximate)\n",
                          framesPerSecond, static_cast<double>(stats.streamElapsedUs) / 1000000.0);
        }
        Serial.printf(" Gyro: %.2f rad/s | Synth tone: %.0f Hz | BMI270: %s\n",
                  sensor.audioSample().angularSpeed, sound.getSynthFrequency(),
                  sensor.connected() ? "connected" : "disconnected");
        Serial.printf(" WAV Storage: %-12s\n", sound.getWavStorage());
        Serial.printf(" WAV Load: %s\n", sound.getWavLoadLog());
        Serial.printf(" Hardware INT1 Pulses: Total: %u | Last Second: %u (%s)\n",
                      currentIsrTotal, isrInLastSec, dataReadyIntActive ? "Continuous Data-Ready" : "Motion-Triggered");
        Serial.println("------------------------------------------------------------------");

        // Latency Metrics
        Serial.printf(" Latency Avg:         %-6lld us  |  Latency Max:         %-6lld us\n",
                      stats.latencyAvgUs, stats.latencyMaxUs);
        Serial.printf("   -> Interrupt -> AudioStart:   %-6lld us\n", stats.lastIsrToStartUs);
        Serial.printf("   -> AudioStart -> FirstBuffer: %-6lld us\n", stats.lastStartToBufUs);
        Serial.printf("   -> Interrupt -> FirstBuffer:  %-6lld us\n", stats.lastTotalLatencyUs);
        Serial.println("------------------------------------------------------------------");

        // DSP Block Execution Metrics
        Serial.printf(" Block Time Avg:      %-6lld us  |  Block Time Max:      %-6lld us\n",
                      stats.blockDurationAvgUs, stats.blockDurationMaxUs);
        Serial.printf(" DSP CPU Load Est:    %5.1f %%    |  Audio Underruns:     %u\n",
                      stats.cpuLoadPercent, stats.underruns);
        Serial.println("------------------------------------------------------------------");

        // Memory Status
        Serial.printf(" Free Internal Heap:  %u KB     |  Free PSRAM:          %u MB / %u MB\n",
                      (unsigned)freeHeap, (unsigned)freePsram, (unsigned)totalPsram);
        Serial.print(" [KEYS] SF2s: ");
        for (int i = 0; i < sound.getSoundFontCount(); i++) {
            Serial.printf("'%d'=%s  ", i + 1, sound.getSoundFontName(i));
        }
        Serial.println("| 's'=SoundFont | 'd'=WavDSP | 'm'=Synth | 'i'=INT | 'r'=Reset | 'p'=Print");
        Serial.println("==================================================================\n");

        sound.resetPeriodicStats();
        lastLogTime = now;
    }
}
