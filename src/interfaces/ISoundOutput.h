#pragma once
#include <cstddef>
#include <cstdint>
#include "actuators/sound/Speaker.h"

class ISoundOutput {
public:
    virtual ~ISoundOutput() = default;
    virtual bool begin(uint32_t sampleRate, int blockSize) = 0;
    virtual bool write(const int16_t* mono, size_t count, const Speakers& speakers,
                       size_t& bytesWritten) = 0;
};
