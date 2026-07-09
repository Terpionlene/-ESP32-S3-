#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ppt_controller.h"

#define TAG "ppt_controller"

#define PPT_TASK_STACK_SIZE 4096
#define PPT_TASK_PRIORITY 3
#define PPT_QUEUE_SIZE 10

static char s_server_ip[16] = "192.168.137.1";
static int s_server_port = 8765;
static bool s_running = false;
static TaskHandle_t s_task_handle = NULL;
static QueueHandle_t s_queue = NULL;
static ppt_exit_cb_t s_exit_cb = NULL;

static const char* action_strings[] = {
    [PPT_ACTION_NEXT] = "next",
    [PPT_ACTION_PREV] = "prev",
    [PPT_ACTION_VOLUME_UP] = "volume_up",
    [PPT_ACTION_VOLUME_DOWN] = "volume_down",
};

static void ppt_task(void *arg)
{
    ppt_action_t action;
    
    while (1) {
        if (xQueueReceive(s_queue, &action, portMAX_DELAY) == pdTRUE) {
            if (action < 0 || action >= sizeof(action_strings) / sizeof(action_strings[0])) {
                ESP_LOGE(TAG, "Invalid action: %d", action);
                continue;
            }

            char url[128];
            snprintf(url, sizeof(url), "http://%s:%d/control?action=%s",
                     s_server_ip, s_server_port, action_strings[action]);

            esp_http_client_config_t config = {
                .url = url,
                .method = HTTP_METHOD_GET,
                .timeout_ms = 3000,
            };

            esp_http_client_handle_t client = esp_http_client_init(&config);
            if (!client) {
                ESP_LOGE(TAG, "Failed to initialize HTTP client");
                continue;
            }

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                int status_code = esp_http_client_get_status_code(client);
                ESP_LOGI(TAG, "Action: %s, Status: %d", action_strings[action], status_code);
            } else {
                ESP_LOGE(TAG, "Failed to send action %s: %s", 
                         action_strings[action], esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
        }
    }
}

void ppt_controller_init(const char* server_ip, int server_port)
{
    if (server_ip) {
        strncpy(s_server_ip, server_ip, sizeof(s_server_ip) - 1);
        s_server_ip[sizeof(s_server_ip) - 1] = '\0';
    }
    if (server_port > 0) {
        s_server_port = server_port;
    }
    ESP_LOGI(TAG, "PPT Controller initialized: %s:%d", s_server_ip, s_server_port);
}

void ppt_controller_set_server(const char* server_ip, int server_port)
{
    if (server_ip) {
        strncpy(s_server_ip, server_ip, sizeof(s_server_ip) - 1);
        s_server_ip[sizeof(s_server_ip) - 1] = '\0';
    }
    if (server_port > 0) {
        s_server_port = server_port;
    }
    ESP_LOGI(TAG, "PPT Server updated: %s:%d", s_server_ip, s_server_port);
}

void ppt_controller_get_server(char* server_ip, int* server_port)
{
    if (server_ip) {
        strcpy(server_ip, s_server_ip);
    }
    if (server_port) {
        *server_port = s_server_port;
    }
}

esp_err_t ppt_controller_send_action(ppt_action_t action)
{
    if (!s_running || !s_queue) {
        ESP_LOGE(TAG, "PPT controller not running");
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_queue, &action, 0) != pdTRUE) {
        ESP_LOGW(TAG, "PPT queue full, dropping action: %d", action);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ppt_controller_start(void)
{
    if (s_running) {
        ESP_LOGW(TAG, "PPT controller already running");
        return;
    }

    if (!s_queue) {
        s_queue = xQueueCreate(PPT_QUEUE_SIZE, sizeof(ppt_action_t));
        if (!s_queue) {
            ESP_LOGE(TAG, "Failed to create PPT queue");
            return;
        }
    }

    if (!s_task_handle) {
        xTaskCreatePinnedToCore(ppt_task, "ppt_task", PPT_TASK_STACK_SIZE, 
                               NULL, PPT_TASK_PRIORITY, &s_task_handle, 0);
        if (!s_task_handle) {
            ESP_LOGE(TAG, "Failed to create PPT task");
            vQueueDelete(s_queue);
            s_queue = NULL;
            return;
        }
    }

    s_running = true;
    ESP_LOGI(TAG, "PPT controller started");
}

void ppt_controller_stop(void)
{
    if (!s_running) {
        return;
    }

    s_running = false;

    if (s_task_handle) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }

    if (s_queue) {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }

    if (s_exit_cb) {
        s_exit_cb();
    }

    ESP_LOGI(TAG, "PPT controller stopped");
}

bool ppt_controller_is_running(void)
{
    return s_running;
}

void ppt_controller_set_exit_callback(ppt_exit_cb_t cb)
{
    s_exit_cb = cb;
}
