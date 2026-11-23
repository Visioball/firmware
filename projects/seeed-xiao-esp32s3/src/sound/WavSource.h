#pragma once
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <LittleFS.h>

// =============================================================================
// WAV Audio Source: Reads a WAV file from LittleFS into PSRAM/RAM for DSP
// =============================================================================
class WavSource {
public:
    WavSource() : ramSamples(nullptr), sampleCount(0), sampleRate(44100), channels(1), bitsPerSample(16), inPsram(false) {
        strcpy(loadLog, "not loaded");
    }

    ~WavSource() {
        freeBuffer();
    }

    // Load a 16-bit PCM WAV file from LittleFS, streaming it into PSRAM in chunks.
    bool loadFromFile(const char* path, bool preferPsram = true) {
        File f = LittleFS.open(path, "r");
        if (!f) {
            snprintf(loadLog, sizeof(loadLog), "FS open FAILED: %s", path);
            Serial.printf("[WAV] %s\n", loadLog);
            return false;
        }

        size_t fileSize = f.size();
        uint8_t header[44];
        if (f.read(header, 44) != 44 ||
            memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
            snprintf(loadLog, sizeof(loadLog), "bad WAV header (size %u)", (unsigned)fileSize);
            Serial.printf("[WAV] %s\n", loadLog);
            f.close();
            return false;
        }

        // Parse fmt fields from the canonical 44-byte header
        uint16_t audioFormat = *(const uint16_t*)(header + 20);
        channels             = *(const uint16_t*)(header + 22);
        sampleRate           = *(const uint32_t*)(header + 24);
        bitsPerSample        = *(const uint16_t*)(header + 34);
        uint32_t dataSize    = *(const uint32_t*)(header + 40);

        if (audioFormat != 1 || bitsPerSample != 16) {
            snprintf(loadLog, sizeof(loadLog), "unsupported fmt=%u bits=%u", audioFormat, bitsPerSample);
            Serial.printf("[WAV] %s\n", loadLog);
            f.close();
            return false;
        }
        if (dataSize == 0 || dataSize > fileSize - 44) dataSize = fileSize - 44;

        freeBuffer();
        size_t totalSamples = dataSize / (sizeof(int16_t) * channels);
        size_t monoBytes = totalSamples * sizeof(int16_t);

        // Size the buffer to the largest free PSRAM block (leave 256 KB margin)
        size_t bufferBytes = 0;
        if (preferPsram && psramFound()) {
            size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
            size_t usable = (largest > 256 * 1024) ? (largest - 256 * 1024) : 0;
            bufferBytes = (monoBytes < usable) ? monoBytes : usable;
            bufferBytes &= ~1u;
            if (bufferBytes >= 2) {
                ramSamples = (int16_t*)heap_caps_malloc(bufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (ramSamples) inPsram = true;
            }
        }
        if (!ramSamples) {
            bufferBytes = (monoBytes < 180 * 1024) ? monoBytes : 180 * 1024; // internal RAM cap
            bufferBytes &= ~1u;
            ramSamples = (int16_t*)heap_caps_malloc(bufferBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            inPsram = false;
        }
        if (!ramSamples) {
            snprintf(loadLog, sizeof(loadLog), "malloc FAILED (%u KB)", (unsigned)(monoBytes / 1024));
            Serial.printf("[WAV] %s\n", loadLog);
            f.close();
            return false;
        }

        sampleCount = bufferBytes / sizeof(int16_t);

        // Read file into an internal-RAM staging buffer, then memcpy to the target buffer.
        // (LittleFS f.read() straight into PSRAM can trigger a cache-write fault on ESP32-S3.)
        const size_t CHUNK = 4096;
        uint8_t* chunk = (uint8_t*)heap_caps_malloc(CHUNK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!chunk) {
            snprintf(loadLog, sizeof(loadLog), "chunk alloc FAILED");
            Serial.printf("[WAV] %s\n", loadLog);
            freeBuffer();
            f.close();
            return false;
        }

        size_t bytesRead = 0;
        if (channels == 1) {
            uint8_t* dst = (uint8_t*)ramSamples;
            while (bytesRead < bufferBytes) {
                size_t want = min(CHUNK, bufferBytes - bytesRead);
                size_t got = f.read(chunk, want);
                if (got == 0) break;
                memcpy(dst + bytesRead, chunk, got);
                bytesRead += got;
            }
        } else {
            size_t written = 0;
            while (written < sampleCount) {
                size_t got = f.read(chunk, CHUNK);
                if (got == 0) break;
                size_t frames = got / (sizeof(int16_t) * 2);
                const int16_t* s = (const int16_t*)chunk;
                for (size_t i = 0; i < frames && written < sampleCount; i++) {
                    ramSamples[written++] = (int16_t)(((int32_t)s[i * 2] + (int32_t)s[i * 2 + 1]) / 2);
                }
                bytesRead += got;
            }
            sampleCount = written;
        }
        free(chunk);
        f.close();

        snprintf(loadLog, sizeof(loadLog), "%s %uKB %uHz %.1fs [%d %d %d]",
                 inPsram ? "PSRAM" : "IntRAM", (unsigned)(bufferBytes / 1024), sampleRate,
                 (float)sampleCount / (float)sampleRate,
                 getSample(0), getSample(44100), getSample(sampleCount / 2));
        Serial.printf("[WAV] Loaded: %s (file %u KB)\n", loadLog, (unsigned)(fileSize / 1024));
        return true;
    }

    bool loadFromMemory(const uint8_t* wavBytes, size_t totalBytes, bool preferPsram = true) {
        if (!wavBytes || totalBytes < 44) {
            Serial.println("[WAV] Error: invalid memory pointer or size too small");
            return false;
        }

        // Validate RIFF header
        if (memcmp(wavBytes, "RIFF", 4) != 0 || memcmp(wavBytes + 8, "WAVE", 4) != 0) {
            Serial.println("[WAV] Error: Not a valid RIFF/WAVE file");
            return false;
        }

        // Parse chunks to locate 'fmt ' and 'data'
        size_t offset = 12;
        bool foundFmt = false;
        bool foundData = false;
        uint16_t audioFormat = 0;
        size_t dataOffset = 0;
        size_t dataSize = 0;

        while (offset + 8 <= totalBytes) {
            char chunkId[5] = {0};
            memcpy(chunkId, wavBytes + offset, 4);
            uint32_t chunkSize = *(const uint32_t*)(wavBytes + offset + 4);
            offset += 8;

            if (strcmp(chunkId, "fmt ") == 0 && chunkSize >= 16) {
                audioFormat   = *(const uint16_t*)(wavBytes + offset);
                channels      = *(const uint16_t*)(wavBytes + offset + 2);
                sampleRate    = *(const uint32_t*)(wavBytes + offset + 4);
                bitsPerSample = *(const uint16_t*)(wavBytes + offset + 14);
                foundFmt = true;
            } else if (strcmp(chunkId, "data") == 0) {
                dataOffset = offset;
                dataSize = (offset + chunkSize <= totalBytes) ? chunkSize : (totalBytes - offset);
                foundData = true;
                break;
            }

            offset += chunkSize;
        }

        if (!foundFmt || !foundData) {
            Serial.println("[WAV] Error: 'fmt ' or 'data' chunk missing");
            return false;
        }

        if (audioFormat != 1 || bitsPerSample != 16) {
            Serial.printf("[WAV] Error: Only 16-bit uncompressed PCM supported (fmt=%u, bits=%u)\n",
                          audioFormat, bitsPerSample);
            return false;
        }

        sampleCount = dataSize / (sizeof(int16_t) * channels);
        freeBuffer();

        size_t monoAllocSize = sampleCount * sizeof(int16_t);
        const int16_t* src = (const int16_t*)(wavBytes + dataOffset);

        Serial.printf("[WAV] Parsed: %u Hz, %u-bit, %u ch, %u samples (%.2f sec, need %u KB)\n",
                      sampleRate, bitsPerSample, channels, (unsigned)sampleCount,
                      (float)sampleCount / (float)sampleRate, (unsigned)(monoAllocSize / 1024));

        if (preferPsram && psramFound()) {
            size_t freeBefore = ESP.getFreePsram();
            size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
            Serial.printf("[WAV] PSRAM free: %u KB | largest block: %u KB\n",
                          (unsigned)(freeBefore / 1024), (unsigned)(largest / 1024));

            // Cap the buffer to the largest available PSRAM block (leave 256 KB margin)
            size_t usableBytes = (largest > (256 * 1024)) ? (largest - 256 * 1024) : 0;
            size_t bufferBytes = (monoAllocSize < usableBytes) ? monoAllocSize : usableBytes;
            bufferBytes &= ~1u; // keep even (whole int16 samples)

            if (bufferBytes >= 2) {
                ramSamples = (int16_t*)heap_caps_malloc(bufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (ramSamples) {
                    inPsram = true;
                    size_t loadedSamples = bufferBytes / sizeof(int16_t);

                    if (channels == 1) {
                        memcpy(ramSamples, src, bufferBytes);
                    } else {
                        for (size_t i = 0; i < loadedSamples; i++) {
                            ramSamples[i] = (int16_t)(((int32_t)src[i * 2] + (int32_t)src[i * 2 + 1]) / 2);
                        }
                    }

                    // Loop only over the samples that actually fit in PSRAM
                    sampleCount = loadedSamples;

                    Serial.printf("[WAV] PSRAM LOADED %u KB (%u samples, %.2f sec) | Free after: %u KB | First: %d %d %d\n",
                                  (unsigned)(bufferBytes / 1024), (unsigned)loadedSamples,
                                  (float)loadedSamples / (float)sampleRate,
                                  (unsigned)(ESP.getFreePsram() / 1024),
                                  getSample(0), getSample(44100), getSample(loadedSamples / 2));
                    return true;
                }
                Serial.printf("[WAV] PSRAM malloc FAILED for %u KB\n", (unsigned)(bufferBytes / 1024));
            }
        }

        // Last resort: play directly from flash-mapped memory (may glitch/return zeros under load)
        Serial.println("[WAV] WARNING: Falling back to FLASH-MAPPED playback (no RAM copy)!");
        ramSamples = (int16_t*)src;
        inPsram = false;
        return true;
    }

    void freeBuffer() {
        if (ramSamples && inPsram) {
            free(ramSamples);
        }
        ramSamples = nullptr;
        sampleCount = 0;
        inPsram = false;
    }

    inline int16_t getSample(size_t index) const {
        if (!ramSamples || sampleCount == 0) return 0;
        return ramSamples[index % sampleCount];
    }

    inline const int16_t* getRawData() const { return ramSamples; }
    inline size_t getSampleCount() const { return sampleCount; }
    inline uint32_t getSampleRate() const { return sampleRate; }
    inline uint16_t getChannels() const { return channels; }
    inline bool isLoadedInPsram() const { return inPsram; }
    inline bool isFlashMapped() const { return ramSamples != nullptr && !inPsram; }
    inline const char* getLoadLog() const { return loadLog; }

private:
    int16_t* ramSamples;
    size_t sampleCount;
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
    bool inPsram;
    char loadLog[96];
};
