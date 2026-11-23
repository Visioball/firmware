#pragma once
#include <Arduino.h>

class Sound;
class Motion;

class DebugLogger {
public:
    DebugLogger(Sound& sound, Motion& motion, volatile uint32_t& isrCounter)
        : sound(sound), motion(motion), hardwareIsrCounter(isrCounter) {}
    void printStartupBanner(int resetReason);
    void printTaskStarted(const char* taskName, int core, int priority);
    void printFileSystemMountFailed();
    void printFileSystemFailure();
    void printFileSystemReady(size_t usedBytes, size_t totalBytes);
    void printMissingAudioTrack(const char* path);
    void printAudioInitializationFailed();
    void printSoundFontStatus(int count);

    void update();
    void handleSerialCommands();

    void togglePrint();
    void setPrintEnabled(bool enabled);
    bool isPrintEnabled() const { return printEnabled; }

    void toggleAutoBlockSize();
    bool isAutoBlockSize() const { return autoToggleBlockSize; }

    void printVolumeChanged(float volumePercent);
    void printModeChanged(const char* modeName);

private:
    Sound& sound;
    Motion& motion;
    volatile uint32_t& hardwareIsrCounter;
    uint32_t lastLogTime = 0;
    uint32_t lastBlockSizeSwitchTime = 0;
    bool printEnabled = false;
    bool autoToggleBlockSize = false;
    bool dataReadyIntActive = false;
    uint32_t prevReportedIsrCount = 0;
};

