#include "SoundFont.h"
#include <math.h>
#include <string.h>

int SoundFont::scanFiles() {
    soundFontCount = 0;
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("[SOUNDFONT SCAN] LittleFS root open failed");
        return 0;
    }

    File file = root.openNextFile();
    while (file && soundFontCount < MAX_SOUNDFONTS) {
        String name = file.name();
        if (!file.isDirectory() && (name.endsWith(".sf2") || name.endsWith(".SF2"))) {
            String path = name.startsWith("/") ? name : "/" + name;
            String displayName = name.startsWith("/") ? name.substring(1) : name;
            strncpy(soundFontPaths[soundFontCount], path.c_str(), sizeof(soundFontPaths[0]) - 1);
            strncpy(soundFontNames[soundFontCount], displayName.c_str(), sizeof(soundFontNames[0]) - 1);
            soundFontCount++;
        }
        file = root.openNextFile();
    }
    root.close();

    for (int first = 0; first < soundFontCount - 1; first++) {
        for (int second = first + 1; second < soundFontCount; second++) {
            if (strcmp(soundFontNames[first], soundFontNames[second]) > 0) {
                char path[64];
                char name[64];
                strcpy(path, soundFontPaths[first]);
                strcpy(name, soundFontNames[first]);
                strcpy(soundFontPaths[first], soundFontPaths[second]);
                strcpy(soundFontNames[first], soundFontNames[second]);
                strcpy(soundFontPaths[second], path);
                strcpy(soundFontNames[second], name);
            }
        }
    }

    Serial.printf("[SOUNDFONT SCAN] Found %d SoundFont files\n", soundFontCount);
    for (int index = 0; index < soundFontCount; index++) {
        Serial.printf("  [%d] %s\n", index + 1, soundFontNames[index]);
    }
    return soundFontCount;
}

bool SoundFont::select(int index) {
    if (index < 0 || index >= soundFontCount) return false;
    return load(soundFontPaths[index]);
}

void SoundFont::update(float movement, float roll, float) {
    const float level = constrain(max(0.0f, movement - 0.4f) / 8.0f, 0.0f, 1.0f);
    setMotionLevel(level);
    setTempo(1.0f + 14.0f * level);
    setPitch(powf(2.0f, constrain(roll / 45.0f, -1.0f, 1.0f)));
}

void SoundFont::render(float* output, int count, float outputRate, float volume) {
    if (isBusyLoading || !sampleData || zoneCount == 0) return;
    currentSampleRate = outputRate;
    masterFilter.setSampleRate(outputRate);

    for (int sample = 0; sample < count; sample++) {
        if (motionLevel > 0.02f) {
            if (samplesUntilNextNote == 0) {
                playNextMotionNote();
                samplesUntilNextNote = (uint32_t)(outputRate * 0.45f / tempo);
                if (samplesUntilNextNote < 1000) samplesUntilNextNote = 1000;
            }
            if (samplesUntilNextNote > 0) samplesUntilNextNote--;
        }

        float mix = 0.0f;
        for (int voiceIndex = 0; voiceIndex < MAX_VOICES; voiceIndex++) {
            SoundFontVoice& voice = voices[voiceIndex];
            if (!voice.active) continue;
            voice.age++;

            switch (voice.envState) {
            case SoundFontVoice::ENV_ATTACK:
                voice.envLevel += voice.attackStep;
                if (voice.envLevel >= 1.0f) {
                    voice.envLevel = 1.0f;
                    voice.envState = voice.holdSamplesLeft > 0
                        ? SoundFontVoice::ENV_HOLD : SoundFontVoice::ENV_DECAY;
                }
                break;
            case SoundFontVoice::ENV_HOLD:
                if (voice.holdSamplesLeft > 0) voice.holdSamplesLeft--;
                else voice.envState = SoundFontVoice::ENV_DECAY;
                break;
            case SoundFontVoice::ENV_DECAY:
                voice.envLevel *= voice.decayFactor;
                if (voice.envLevel <= voice.sustainLevel) {
                    voice.envLevel = voice.sustainLevel;
                    if (voice.sustainLevel <= 0.001f) voice.envState = SoundFontVoice::ENV_RELEASE;
                }
                break;
            case SoundFontVoice::ENV_SUSTAIN:
                break;
            case SoundFontVoice::ENV_RELEASE:
                voice.envLevel *= voice.releaseFactor;
                if (voice.envLevel <= 0.0005f) {
                    voice.envLevel = 0.0f;
                    voice.envState = SoundFontVoice::ENV_IDLE;
                    voice.active = false;
                    continue;
                }
                break;
            default:
                voice.active = false;
                continue;
            }

            uint32_t sampleIndex = voice.sampleStart + (uint32_t)voice.samplePosition;
            if (sampleIndex >= voice.sampleEnd) {
                voice.active = false;
                continue;
            }

            float fraction = (float)(voice.samplePosition - floor(voice.samplePosition));
            int16_t sample0 = sampleData[sampleIndex];
            int16_t sample1 = sampleIndex + 1 < voice.sampleEnd ? sampleData[sampleIndex + 1] : sample0;
            float value = ((float)sample0 + fraction * (float)(sample1 - sample0)) / 32768.0f;
            mix += voice.filter.process(value) * voice.envLevel * voice.voiceGain;

            voice.samplePosition += voice.pitchRatio;
            if (voice.loopMode == 1 && voice.sampleStart + (uint32_t)voice.samplePosition >= voice.loopEnd) {
                voice.samplePosition -= (double)(voice.loopEnd - voice.loopStart);
            } else if (voice.sampleStart + (uint32_t)voice.samplePosition >= voice.sampleEnd) {
                voice.active = false;
            }
        }
        output[sample] += masterFilter.process(mix) * volume;
    }
}

size_t SoundFont::findChunk(const char* name, size_t startOffset) const {
    if (!fileData || fileSize < 8) return SIZE_MAX;
    for (size_t offset = startOffset; offset + 8 <= fileSize; offset++) {
        if (memcmp(fileData + offset, name, 4) == 0) return offset;
    }
    return SIZE_MAX;
}

size_t SoundFont::findListChunk(const char* listType) const {
    if (!fileData || fileSize < 12) return SIZE_MAX;
    size_t position = 12;
    while (position + 12 <= fileSize) {
        if (memcmp(fileData + position, "LIST", 4) == 0) {
            uint32_t listSize = *(const uint32_t*)(fileData + position + 4);
            if (memcmp(fileData + position + 8, listType, 4) == 0) return position + 8;
            position += 8 + listSize;
            if (listSize % 2 == 1) position++;
        } else {
            uint32_t size = *(const uint32_t*)(fileData + position + 4);
            position += 8 + size;
            if (size % 2 == 1) position++;
        }
    }
    return SIZE_MAX;
}

size_t SoundFont::findSubChunk(size_t parentOffset, size_t parentSize, const char* name,
                               uint32_t* outSize) const {
    if (!fileData || parentOffset + 4 > fileSize) return SIZE_MAX;
    size_t position = parentOffset + 4;
    size_t end = min(parentOffset + parentSize, fileSize);
    while (position + 8 <= end) {
        uint32_t size = *(const uint32_t*)(fileData + position + 4);
        if (memcmp(fileData + position, name, 4) == 0) {
            if (outSize) *outSize = size;
            return position + 8;
        }
        position += 8 + size;
        if (size % 2 == 1) position++;
    }
    return SIZE_MAX;
}

bool SoundFont::parseSoundFont() {
    zoneCount = 0;
    size_t sampleList = findListChunk("sdta");
    if (sampleList == SIZE_MAX) {
        size_t sampleChunk = findChunk("smpl", 0);
        if (sampleChunk == SIZE_MAX) return false;
        sampleData = (const int16_t*)(fileData + sampleChunk + 8);
    } else {
        uint32_t listSize = *(const uint32_t*)(fileData + sampleList - 4);
        uint32_t sampleSize = 0;
        size_t sampleChunk = findSubChunk(sampleList, listSize, "smpl", &sampleSize);
        if (sampleChunk == SIZE_MAX) return false;
        sampleData = (const int16_t*)(fileData + sampleChunk);
    }

    size_t presetList = findListChunk("pdta");
    if (presetList == SIZE_MAX) return false;
    uint32_t listSize = *(const uint32_t*)(fileData + presetList - 4);
    uint32_t phdrSize = 0, pbagSize = 0, pgenSize = 0;
    uint32_t instSize = 0, ibagSize = 0, igenSize = 0, shdrSize = 0;
    size_t phdrOffset = findSubChunk(presetList, listSize, "phdr", &phdrSize);
    size_t pbagOffset = findSubChunk(presetList, listSize, "pbag", &pbagSize);
    size_t pgenOffset = findSubChunk(presetList, listSize, "pgen", &pgenSize);
    size_t instOffset = findSubChunk(presetList, listSize, "inst", &instSize);
    size_t ibagOffset = findSubChunk(presetList, listSize, "ibag", &ibagSize);
    size_t igenOffset = findSubChunk(presetList, listSize, "igen", &igenSize);
    size_t shdrOffset = findSubChunk(presetList, listSize, "shdr", &shdrSize);
    if (phdrOffset == SIZE_MAX || instOffset == SIZE_MAX || ibagOffset == SIZE_MAX ||
        igenOffset == SIZE_MAX || shdrOffset == SIZE_MAX) return false;

    const SF2Phdr* phdrs = (const SF2Phdr*)(fileData + phdrOffset);
    const SF2Bag* pbags = (const SF2Bag*)(fileData + pbagOffset);
    const SF2Gen* pgens = (const SF2Gen*)(fileData + pgenOffset);
    const SF2Inst* instruments = (const SF2Inst*)(fileData + instOffset);
    const SF2Bag* ibags = (const SF2Bag*)(fileData + ibagOffset);
    const SF2Gen* igens = (const SF2Gen*)(fileData + igenOffset);
    const SF2Shdr* headers = (const SF2Shdr*)(fileData + shdrOffset);
    int presetCount = phdrSize / sizeof(SF2Phdr);
    int instrumentCount = instSize / sizeof(SF2Inst);
    int instrumentBagCount = ibagSize / sizeof(SF2Bag);
    int instrumentGenCount = igenSize / sizeof(SF2Gen);
    int sampleHeaderCount = shdrSize / sizeof(SF2Shdr);
    if (presetCount < 2 || instrumentCount < 2 || sampleHeaderCount < 2) return false;

    int targetInstrument = 0;
    if (pbagOffset != SIZE_MAX && pgenOffset != SIZE_MAX) {
        uint16_t bagStart = phdrs[0].presetBagNdx;
        uint16_t bagEnd = phdrs[1].presetBagNdx;
        for (uint16_t bag = bagStart; bag < bagEnd && bag < pbagSize / sizeof(SF2Bag); bag++) {
            uint16_t genStart = pbags[bag].genNdx;
            uint16_t genEnd = pbags[bag + 1].genNdx;
            for (uint16_t gen = genStart; gen < genEnd && gen < pgenSize / sizeof(SF2Gen); gen++) {
                if (pgens[gen].genOper == 41) {
                    targetInstrument = (uint16_t)pgens[gen].genAmount;
                    break;
                }
            }
        }
    }
    if (targetInstrument >= instrumentCount - 1) targetInstrument = 0;

    uint16_t bagStart = instruments[targetInstrument].instBagNdx;
    uint16_t bagEnd = min((int)instruments[targetInstrument + 1].instBagNdx, instrumentBagCount);
    int16_t globalGens[60];
    bool hasGlobalGen[60];
    memset(hasGlobalGen, 0, sizeof(hasGlobalGen));
    uint16_t firstBag = bagStart;

    if (bagStart < bagEnd) {
        uint16_t genStart = ibags[bagStart].genNdx;
        uint16_t genEnd = ibags[bagStart + 1].genNdx;
        bool hasSample = false;
        for (uint16_t gen = genStart; gen < genEnd && gen < instrumentGenCount; gen++) {
            if (igens[gen].genOper == 53) hasSample = true;
        }
        if (!hasSample) {
            for (uint16_t gen = genStart; gen < genEnd && gen < instrumentGenCount; gen++) {
                if (igens[gen].genOper < 60) {
                    globalGens[igens[gen].genOper] = igens[gen].genAmount;
                    hasGlobalGen[igens[gen].genOper] = true;
                }
            }
            firstBag++;
        }
    }

    for (uint16_t bag = firstBag; bag < bagEnd && zoneCount < MAX_ZONES; bag++) {
        uint16_t genStart = ibags[bag].genNdx;
        uint16_t genEnd = ibags[bag + 1].genNdx;
        int16_t localGens[60];
        bool hasGen[60];
        memcpy(localGens, globalGens, sizeof(localGens));
        memcpy(hasGen, hasGlobalGen, sizeof(hasGen));
        for (uint16_t gen = genStart; gen < genEnd && gen < instrumentGenCount; gen++) {
            if (igens[gen].genOper < 60) {
                localGens[igens[gen].genOper] = igens[gen].genAmount;
                hasGen[igens[gen].genOper] = true;
            }
        }
        if (!hasGen[53]) continue;
        uint16_t sampleId = (uint16_t)localGens[53];
        if (sampleId >= sampleHeaderCount - 1) continue;

        const SF2Shdr& header = headers[sampleId];
        SoundFontZone& zone = zones[zoneCount++];
        if (hasGen[43]) {
            zone.loKey = (uint8_t)(localGens[43] & 0xFF);
            zone.hiKey = (uint8_t)((localGens[43] >> 8) & 0xFF);
        }
        if (hasGen[44]) {
            zone.loVel = (uint8_t)(localGens[44] & 0xFF);
            zone.hiVel = (uint8_t)((localGens[44] >> 8) & 0xFF);
        }
        zone.rootKey = hasGen[58] ? (uint8_t)localGens[58] : header.originalPitch;
        zone.coarseTune = hasGen[51] ? localGens[51] : 0;
        zone.fineTune = (hasGen[52] ? localGens[52] : 0) + header.pitchCorrection;
        int32_t startOffset = (hasGen[0] ? localGens[0] : 0) + ((hasGen[4] ? localGens[4] : 0) << 15);
        int32_t endOffset = (hasGen[1] ? localGens[1] : 0) + ((hasGen[12] ? localGens[12] : 0) << 15);
        int32_t loopStartOffset = (hasGen[2] ? localGens[2] : 0) + ((hasGen[45] ? localGens[45] : 0) << 15);
        int32_t loopEndOffset = (hasGen[3] ? localGens[3] : 0) + ((hasGen[50] ? localGens[50] : 0) << 15);
        zone.sampleStart = header.start + startOffset;
        zone.sampleEnd = header.end + endOffset;
        zone.loopStart = header.startLoop + loopStartOffset;
        zone.loopEnd = header.endLoop + loopEndOffset;
        zone.sampleRate = header.sampleRate ? header.sampleRate : 44100;
        zone.loopMode = hasGen[54] ? (uint8_t)localGens[54] : 0;
        zone.pan = hasGen[17] ? (float)localGens[17] / 1000.0f : 0.0f;
        int16_t attenuation = hasGen[48] ? localGens[48] : 0;
        zone.attenuation = powf(10.0f, -(float)attenuation * 0.1f / 20.0f);
        int16_t attack = hasGen[34] ? localGens[34] : -12000;
        int16_t hold = hasGen[35] ? localGens[35] : -12000;
        int16_t decay = hasGen[36] ? localGens[36] : -12000;
        int16_t sustain = hasGen[37] ? localGens[37] : 1000;
        int16_t release = hasGen[38] ? localGens[38] : -12000;
        zone.attackSec = timecentsToSeconds(attack);
        zone.holdSec = hold > -11950 ? timecentsToSeconds(hold) : 0.0f;
        zone.decaySec = decay > -11950 ? timecentsToSeconds(decay) : 5.0f;
        zone.sustainLevel = sustain >= 1000 ? 0.0f : powf(10.0f, -(float)sustain * 0.1f / 20.0f);
        zone.releaseSec = release > -11950 ? timecentsToSeconds(release) : 0.4f;
        if (zone.releaseSec < 0.08f) zone.releaseSec = 0.4f;
        zone.initialFilterCutoff = hasGen[8]
            ? 8.176f * powf(2.0f, (float)localGens[8] / 1200.0f) : 18000.0f;
    }
    return zoneCount > 0;
}

int SoundFont::allocateVoice() {
    for (int voice = 0; voice < MAX_VOICES; voice++) {
        if (!voices[voice].active) return voice;
    }
    float lowestLevel = 999.0f;
    int bestIndex = -1;
    for (int voice = 0; voice < MAX_VOICES; voice++) {
        if (voices[voice].envState == SoundFontVoice::ENV_RELEASE && voices[voice].envLevel < lowestLevel) {
            lowestLevel = voices[voice].envLevel;
            bestIndex = voice;
        }
    }
    if (bestIndex >= 0) return bestIndex;
    uint32_t oldestAge = 0;
    for (int voice = 0; voice < MAX_VOICES; voice++) {
        if (voices[voice].age > oldestAge) {
            oldestAge = voices[voice].age;
            bestIndex = voice;
        }
    }
    return bestIndex >= 0 ? bestIndex : 0;
}

void SoundFont::playNextMotionNote() {
    static const int notes[] = { 48, 52, 55, 57, 60, 64, 67, 69, 72, 76, 79, 81, 84 };
    static constexpr int NOTE_COUNT = sizeof(notes) / sizeof(notes[0]);
    static int noteStep = 0;
    int pitchOffset = (int)roundf(log2f(pitch) * 12.0f);
    int note = constrain(notes[(noteStep++) % NOTE_COUNT] + pitchOffset, 36, 96);
    noteOn(note, velocity);
}

int SoundFont::currentMidi() const {
    return constrain(60 + (int)roundf(log2f(pitch) * 12.0f), 36, 96);
}