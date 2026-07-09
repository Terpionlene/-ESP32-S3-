#include "driver/gpio.h"
#include "board_pin.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl/lvgl.h"
#include "lvgl_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ppt_controller.h"

#define TAG "LVGL_INPUT"

static int32_t encoder_diff = 0;
static bool btn_pressed_raw = false;
static bool btn_released_raw = false;

static uint8_t encoder_last_state = 0;
static bool ppt_control_mode = false;
static uint32_t encoder_last_time = 0;
#define ENCODER_DEBOUNCE_US 5000

static uint32_t btn_last_time = 0;
#define BTN_DEBOUNCE_US 20000

static int32_t encoder_accumulator = 0;
#define ENCODER_STEP 5

static lv_indev_t *indev_encoder = NULL;
static lv_group_t *group = NULL;

static ppt_exit_cb_t s_ppt_exit_cb = NULL;

static uint32_t btn_press_start_time = 0;
static bool btn_long_press_detected = false;
#define BTN_LONG_PRESS_US 1000000

static uint32_t last_release_time = 0;
static uint8_t click_count = 0;
#define DOUBLE_CLICK_US 300000

void lvgl_input_read(lv_indev_t *indev, lv_indev_data_t *data);

static void IRAM_ATTR iram_handler(void* arg)
{
    uint32_t now = esp_timer_get_time();
    if (now - encoder_last_time < ENCODER_DEBOUNCE_US) return;
    encoder_last_time = now;
    
    uint8_t a = gpio_get_level(ENCODER_A_PIN);
    uint8_t b = gpio_get_level(ENCODER_B_PIN);
    uint8_t current_state = (a << 1) | b;
    
    uint8_t transition = (encoder_last_state << 2) | current_state;
    switch(transition) {
        case 0b0001: case 0b0111: case 0b1110: case 0b1000:
            encoder_diff--;
            break;
        case 0b0010: case 0b1011: case 0b1101: case 0b0100:
            encoder_diff++;
            break;
    }
    
    encoder_last_state = current_state;
}

static void IRAM_ATTR btn_iram_handler(void* arg)
{
    uint32_t now = esp_timer_get_time();
    if (now - btn_last_time < BTN_DEBOUNCE_US) return;
    btn_last_time = now;
    
    int level = gpio_get_level(ENCODER_BTN_PIN);
    if (level == 0) {
        btn_pressed_raw = true;
        btn_released_raw = false;
    } else {
        btn_released_raw = true;
    }
}

void lvgl_input_init(void)
{
    gpio_config_t enc_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENCODER_A_PIN) | (1ULL << ENCODER_B_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&enc_conf));
    
    gpio_config_t btn_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENCODER_BTN_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_conf));
    
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_A_PIN, iram_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_B_PIN, iram_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_BTN_PIN, btn_iram_handler, NULL));
    
    group = lv_group_create();
    
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, lvgl_input_read);
    lv_indev_set_group(indev_encoder, group);
    
    ESP_LOGI(TAG, "Input initialized");
}

static bool btn_is_pressed = false;

static void handle_ppt_btn_press(void)
{
    uint32_t now = esp_timer_get_time();
    
    if (btn_pressed_raw) {
        btn_pressed_raw = false;
        btn_is_pressed = true;
        btn_press_start_time = now;
        btn_long_press_detected = false;
        return;
    }
    
    if (btn_released_raw) {
        btn_released_raw = false;
        
        if (!btn_is_pressed) {
            return;
        }
        
        btn_is_pressed = false;
        
        if (btn_long_press_detected) {
            btn_long_press_detected = false;
            return;
        }
        
        uint32_t press_duration = now - btn_press_start_time;
        
        uint32_t time_since_last_release = now - last_release_time;
        if (time_since_last_release < DOUBLE_CLICK_US) {
            click_count++;
        } else {
            click_count = 1;
        }
        last_release_time = now;
        
        if (click_count >= 2) {
            ESP_LOGI(TAG, "Double click - volume down");
            ppt_controller_send_action(PPT_ACTION_VOLUME_DOWN);
            click_count = 0;
        } else {
            ESP_LOGI(TAG, "Single click - volume up");
            ppt_controller_send_action(PPT_ACTION_VOLUME_UP);
        }
        return;
    }
    
    if (btn_is_pressed && !btn_long_press_detected) {
        uint32_t press_duration = now - btn_press_start_time;
        if (press_duration >= BTN_LONG_PRESS_US) {
            btn_long_press_detected = true;
            ESP_LOGI(TAG, "Long press detected - exiting PPT mode");
            lvgl_input_set_ppt_mode(false);
            ppt_controller_stop();
            if (s_ppt_exit_cb) {
                s_ppt_exit_cb();
            }
        }
    }
}

static int32_t ppt_encoder_accum = 0;
#define PPT_ENCODER_STEP 3

void lvgl_input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (ppt_control_mode) {
        if (encoder_diff != 0) {
            ppt_encoder_accum += encoder_diff;
            encoder_diff = 0;
            
            if (ppt_encoder_accum >= PPT_ENCODER_STEP) {
                int steps = ppt_encoder_accum / PPT_ENCODER_STEP;
                for (int i = 0; i < steps; i++) {
                    esp_err_t ret = ppt_controller_send_action(PPT_ACTION_NEXT);
                    ESP_LOGI(TAG, "Send NEXT action: %s", esp_err_to_name(ret));
                }
                ppt_encoder_accum %= PPT_ENCODER_STEP;
            } else if (ppt_encoder_accum <= -PPT_ENCODER_STEP) {
                int steps = (-ppt_encoder_accum) / PPT_ENCODER_STEP;
                for (int i = 0; i < steps; i++) {
                    esp_err_t ret = ppt_controller_send_action(PPT_ACTION_PREV);
                    ESP_LOGI(TAG, "Send PREV action: %s", esp_err_to_name(ret));
                }
                ppt_encoder_accum = -((-ppt_encoder_accum) % PPT_ENCODER_STEP);
            }
        }
        
        handle_ppt_btn_press();
        
        data->enc_diff = 0;
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        ppt_encoder_accum = 0;
        encoder_accumulator += encoder_diff;
        encoder_diff = 0;
        
        data->enc_diff = encoder_accumulator / ENCODER_STEP;
        encoder_accumulator %= ENCODER_STEP;
        
        if (btn_pressed_raw) {
            data->state = LV_INDEV_STATE_PRESSED;
            btn_pressed_raw = false;
            btn_released_raw = false;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }
}

void lvgl_input_set_ppt_mode(bool enable)
{
    ppt_control_mode = enable;
    if (enable) {
        btn_is_pressed = false;
        btn_pressed_raw = false;
        btn_released_raw = false;
        btn_press_start_time = 0;
        btn_long_press_detected = false;
        click_count = 0;
        last_release_time = 0;
        encoder_diff = 0;
        ppt_encoder_accum = 0;
    }
    ESP_LOGI(TAG, "PPT control mode %s", enable ? "enabled" : "disabled");
}

void lvgl_input_deinit(void)
{
    gpio_isr_handler_remove(ENCODER_A_PIN);
    gpio_isr_handler_remove(ENCODER_B_PIN);
    gpio_isr_handler_remove(ENCODER_BTN_PIN);
    gpio_uninstall_isr_service();
}

lv_group_t *lvgl_input_get_group(void)
{
    return group;
}

lv_indev_t *lvgl_input_get_indev(void)
{
    return indev_encoder;
}

void lvgl_input_set_ppt_exit_callback(ppt_exit_cb_t cb)
{
    s_ppt_exit_cb = cb;
}
