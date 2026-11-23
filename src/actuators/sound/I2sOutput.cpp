#include "I2sOutput.h"
#include "../../configuration/config.h"

I2sOutput::~I2sOutput() {
    if (channel) {
        i2s_channel_disable(channel);
        i2s_del_channel(channel);
    }
}

bool I2sOutput::begin(uint32_t sampleRate, int blockSize) {
    i2s_chan_config_t config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    config.dma_desc_num = 4;
    config.dma_frame_num = blockSize;
    config.auto_clear = true;
    esp_err_t error = i2s_new_channel(&config, &channel, nullptr);
    if (error == ESP_OK) {
        i2s_std_config_t format = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                         I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = static_cast<gpio_num_t>(PIN_I2S_BCK),
                .ws = static_cast<gpio_num_t>(PIN_I2S_WS),
                .dout = static_cast<gpio_num_t>(PIN_I2S_DATA),
                .din = I2S_GPIO_UNUSED,
                .invert_flags = {},
            },
        };
        format.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
        error = i2s_channel_init_std_mode(channel, &format);
    }
    if (error == ESP_OK) error = i2s_channel_enable(channel);
    if (error != ESP_OK) Serial.printf("[SOUND] I2S initialization failed (%d)\n", error);
    return error == ESP_OK;
}

bool I2sOutput::write(const int16_t* mono, size_t count, const Speakers&, size_t& bytesWritten) {
    // Compatibility output duplicates mono; six discrete outputs require the PCM1681 driver.
    int16_t stereo[512];
    if (count > 256 || !channel) {
        Serial.println("[SOUND] Invalid I2S block or uninitialized output");
        return false;
    }
    for (size_t i = 0; i < count; ++i) stereo[i * 2] = stereo[i * 2 + 1] = mono[i];
    const size_t expected = count * 2 * sizeof(int16_t);
    const esp_err_t error = i2s_channel_write(channel, stereo, expected, &bytesWritten, portMAX_DELAY);
    if (error != ESP_OK || bytesWritten != expected) {
        Serial.printf("[SOUND] I2S write failed (%d, %u/%u bytes)\n", error,
                      static_cast<unsigned>(bytesWritten), static_cast<unsigned>(expected));
        return false;
    }
    return true;
}
