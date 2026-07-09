#include "audio_es8311.h"
#include "board_pin.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <math.h>

#define TAG "audio-es8311"
#define ES8311_ADDR 0x10
#define I2S_SAMPLE_RATE 16000

static i2s_chan_handle_t i2s_rx_handle = NULL;
static i2s_chan_handle_t i2s_tx_handle = NULL;
static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t es8311_dev = NULL;

static esp_err_t es8311_write(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    return i2c_master_transmit(es8311_dev, data, sizeof(data), -1);
}

void es8311_codec_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_ES8311_SDA,
        .scl_io_num = PIN_ES8311_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8311_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &es8311_dev));
    ESP_LOGI(TAG, "I2C master initialized");
    ESP_LOGI(TAG, "ES8311 device added");
    
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "I2C bus ready, initializing ES8311");

    es8311_write(0x00, 0x1F);
    vTaskDelay(pdMS_TO_TICKS(10));
    es8311_write(0x00, 0x00);

    es8311_write(0x01, 0x3F);
    es8311_write(0x03, 0x08);
    es8311_write(0x04, 0x02);

    es8311_write(0x08, 0x80);
    es8311_write(0x09, 0x88);

    es8311_write(0x12, 0x01);
    es8311_write(0x13, 0x00);

    es8311_write(0x14, 0x00);
    es8311_write(0x15, 0x00);

    es8311_write(0x16, 0x00);
    es8311_write(0x17, 0x00);
    es8311_write(0x18, 0x00);
    es8311_write(0x19, 0x00);

    es8311_write(0x1A, 0x00);
    es8311_write(0x1B, 0x00);
    es8311_write(0x1C, 0x05);
    es8311_write(0x1D, 0x00);

    es8311_write(0x0A, 0x82);
    es8311_write(0x0B, 0x00);

    es8311_write(0x0D, 0x01);
    es8311_write(0x0E, 0x02);

    es8311_write(0x1E, 0x00);
    es8311_write(0x1F, 0x00);

    es8311_write(0x20, 0x00);
    es8311_write(0x21, 0x80);

    es8311_write(0x22, 0x14);
    es8311_write(0x23, 0x14);

    es8311_write(0x00, 0x80);

    vTaskDelay(pdMS_TO_TICKS(20));

    ESP_LOGI(TAG, "ES8311 codec initialized with MIC input");
}

void audio_i2s_init(void)
{
    gpio_config_t ns4150b_en_cfg = {
        .pin_bit_mask = (1ULL << PIN_NS4150B_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ns4150b_en_cfg);
    gpio_set_level(PIN_NS4150B_EN, 1);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    i2s_new_channel(&chan_cfg, &i2s_tx_handle, &i2s_rx_handle);

    i2s_std_config_t i2s_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = PIN_ES8311_MCK,
            .bclk = PIN_ES8311_BCK,
            .ws = PIN_ES8311_WS,
            .dout = PIN_ES8311_DO,
            .din = PIN_ES8311_DI,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    i2s_channel_init_std_mode(i2s_tx_handle, &i2s_std_cfg);
    i2s_channel_init_std_mode(i2s_rx_handle, &i2s_std_cfg);

    i2s_channel_enable(i2s_tx_handle);
    i2s_channel_enable(i2s_rx_handle);

    ESP_LOGI(TAG, "I2S initialized");
}

void audio_play_buffer(int16_t *buffer, size_t len)
{
    size_t bytes_written;
    i2s_channel_write(i2s_tx_handle, buffer, len * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

void audio_start_recording(void)
{
}

void audio_stop_recording(int16_t *buffer, size_t *len)
{
}

void audio_set_volume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    uint8_t vol_value = (uint8_t)(volume * 0xFE / 100);
    
    es8311_write(0x10, vol_value);
    es8311_write(0x11, vol_value);
    
    ESP_LOGI(TAG, "Volume set to %d%% (reg: 0x%02X)", volume, vol_value);
}

void audio_set_mute(bool mute)
{
    uint8_t dac_ctrl = mute ? 0x00 : 0x80;
    
    es8311_write(0x08, dac_ctrl);
    
    ESP_LOGI(TAG, "Mute %s", mute ? "ON" : "OFF");
}

void audio_read_samples(int16_t *buffer, size_t max_samples, size_t *read_samples)
{
    size_t bytes_read;
    i2s_channel_read(i2s_rx_handle, buffer, max_samples * sizeof(int16_t), &bytes_read, 100);
    *read_samples = bytes_read / sizeof(int16_t);
    ESP_LOGI(TAG, "audio_read_samples: requested %zu, read %zu samples (%zu bytes)", max_samples, *read_samples, bytes_read);
}