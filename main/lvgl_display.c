#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board_pin.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl/lvgl.h"

#define TAG "lvgl-display"

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

void lvgl_display_init(void)
{
    ESP_LOGI(TAG, "Starting display init");

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_BLK_PIN
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_BLK_PIN, 0);

    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SCK_PIN,
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = LCD_MISO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC_PIN,
        .cs_gpio_num = LCD_CS_PIN,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RES_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, LCD_ROW_OFFSET));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, 0x53, (uint8_t[]){0x20}, 1));
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, 0x51, (uint8_t[]){128}, 1));

    gpio_set_level(LCD_BLK_PIN, 1);

    ESP_LOGI(TAG, "Display initialized");
}

void lvgl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

void lvgl_display_deinit(void)
{
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
    }
    spi_bus_free(SPI2_HOST);
}

void lvgl_display_set_brightness(int brightness)
{
    if (!io_handle) {
        ESP_LOGE(TAG, "IO handle not initialized");
        return;
    }

    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;

    uint8_t brightness_value = (uint8_t)(brightness * 255 / 100);

    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, 0x53, (uint8_t[]){0x20}, 1));

    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, 0x51, &brightness_value, 1));

    ESP_LOGI(TAG, "Brightness set to %d%% (%d)", brightness, brightness_value);
}