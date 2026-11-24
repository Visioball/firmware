#pragma once
#include <driver/i2s_std.h>
#include "interfaces/ISoundOutput.h"

class I2sOutput final : public ISoundOutput {
public:
    ~I2sOutput();
    bool begin(uint32_t sampleRate, int blockSize) override;
    bool write(const int16_t* mono, size_t count, const Speakers& speakers,
               size_t& bytesWritten) override;

private:
    i2s_chan_handle_t channel = nullptr;
};
