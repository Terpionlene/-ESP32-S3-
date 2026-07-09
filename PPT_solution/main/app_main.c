#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_http_client.h"

#define TAG "ppt_controller"

#define WIFI_SSID "Honor 400 Pro"
#define WIFI_PASSWORD "lmz,0421"
#define SERVER_IP "192.168.1.100"
#define SERVER_PORT 8765

#define EC11_A_PIN 1
#define EC11_B_PIN 2
#define EC11_SW_PIN 4

#define HTTP_GET_URL "http://%s:%d/control?action=%s"

typedef enum {
    ACTION_NONE = 0,
    ACTION_NEXT,
    ACTION_PREV,
    ACTION_VOLUME_UP,
    ACTION_VOLUME_DOWN,
} control_action_t;

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static int s_wifi_retry_num = 0;

static volatile bool wifi_connected = false;
static QueueHandle_t s_control_queue = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        if (s_wifi_retry_num < 5) {
            esp_wifi_connect();
            s_wifi_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "WiFi disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_retry_num = 0;
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP successfully");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to AP");
    } else {
        ESP_LOGE(TAG, "Unexpected event");
    }
}

static esp_err_t send_control_request(const char* action)
{
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi not connected, cannot send request");
        return ESP_ERR_NOT_CONNECTED;
    }

    char url[128];
    snprintf(url, sizeof(url), HTTP_GET_URL, SERVER_IP, SERVER_PORT, action);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Request sent: %s, Status code: %d", action, status_code);
    } else {
        ESP_LOGE(TAG, "Failed to send request: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

static void IRAM_ATTR ec11_isr_handler(void *arg)
{
    static uint8_t a_prev = 1;
    static uint32_t last_rotate_time = 0;
    uint32_t current_time = xTaskGetTickCountFromISR();

    uint8_t a = gpio_get_level(EC11_A_PIN);

    if (a_prev == 1 && a == 0) {
        if (current_time >= last_rotate_time + pdMS_TO_TICKS(15)) {
            uint8_t b = gpio_get_level(EC11_B_PIN);
            control_action_t action = (b == 1) ? ACTION_NEXT : ACTION_PREV;
            xQueueSendFromISR(s_control_queue, &action, NULL);
            last_rotate_time = current_time;
        }
    }

    a_prev = a;

    static uint8_t sw_prev = 1;
    static uint32_t last_press_time = 0;
    uint8_t sw = gpio_get_level(EC11_SW_PIN);
    if (sw != sw_prev) {
        if (current_time >= last_press_time + pdMS_TO_TICKS(50)) {
            if (sw == 0) {
                control_action_t action = ACTION_VOLUME_UP;
                xQueueSendFromISR(s_control_queue, &action, NULL);
            }
            last_press_time = current_time;
        }
        sw_prev = sw;
    }
}

static void init_ec11_pins(void)
{
    gpio_config_t ec11_gpio_config = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << EC11_A_PIN) | (1ULL << EC11_B_PIN) | (1ULL << EC11_SW_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&ec11_gpio_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_A_PIN, ec11_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_B_PIN, ec11_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_SW_PIN, ec11_isr_handler, NULL));

    ESP_LOGI(TAG, "EC11 encoder pins initialized: A=%d B=%d SW=%d",
             EC11_A_PIN, EC11_B_PIN, EC11_SW_PIN);
}

static void control_task(void *arg)
{
    control_action_t action;

    while (1) {
        if (xQueueReceive(s_control_queue, &action, portMAX_DELAY) == pdTRUE) {
            switch (action) {
                case ACTION_NEXT:
                    ESP_LOGI(TAG, "Action: Next Page");
                    send_control_request("next");
                    break;
                case ACTION_PREV:
                    ESP_LOGI(TAG, "Action: Previous Page");
                    send_control_request("prev");
                    break;
                case ACTION_VOLUME_UP:
                    ESP_LOGI(TAG, "Action: Volume Up");
                    send_control_request("volume_up");
                    break;
                case ACTION_VOLUME_DOWN:
                    ESP_LOGI(TAG, "Action: Volume Down");
                    send_control_request("volume_down");
                    break;
                default:
                    break;
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "PPT Controller - WiFi Version");
    ESP_LOGI(TAG, "==========================================");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_control_queue = xQueueCreate(10, sizeof(control_action_t));
    if (!s_control_queue) {
        ESP_LOGE(TAG, "Failed to create control queue");
        return;
    }

    init_ec11_pins();

    wifi_init_sta();

    xTaskCreate(control_task, "control_task", 4096, NULL, 5, NULL);

    while (1) {
        if (wifi_connected) {
            ESP_LOGI(TAG, "WiFi connected, ready to control PPT");
        } else {
            ESP_LOGI(TAG, "WiFi not connected");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}