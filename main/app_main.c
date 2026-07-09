#include <string.h>
#include <stddef.h>
#include <sys/lock.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "mbedtls/base64.h"
#include "audio_es8311.h"
#include "voice_chat.h"
#include "lvgl/lvgl.h"
#include "lvgl_display.h"
#include "lvgl_input.h"
#include "lvgl_ui.h"
#include "icons.h"
#include "ppt_controller.h"
#include "test_audio.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define WIFI_SSID "6283"
#define WIFI_PASSWORD "l1234567"

char wifi_ssid[32] = WIFI_SSID;
char wifi_password[64] = WIFI_PASSWORD;

#define DEEPSEEK_API_KEY CONFIG_DEEPSEEK_API_KEY
#define DEEPSEEK_API_URL "https://api.deepseek.com/v1/chat/completions"

#define BAIDU_API_KEY CONFIG_BAIDU_API_KEY
#define BAIDU_SECRET_KEY CONFIG_BAIDU_SECRET_KEY
#define BAIDU_ASR_URL "http://vop.baidu.com/server_api"

#define CONFIG_WIFI_MAXIMUM_RETRY 5

#define MAX_RESPONSE_SIZE 4096
#define MAX_WAV_SIZE 64000

char wifi_ip[16] = "0.0.0.0";
bool wifi_connected = false;
int sound_volume = 50;
bool sound_mute = false;
int lcd_brightness = 50;
char deepseek_api_key[128] = DEEPSEEK_API_KEY;
char baidu_access_token[256] = {0};
uint32_t baidu_token_expire_time = 0;

EXT_RAM_BSS_ATTR static int16_t g_wav_buffer[MAX_WAV_SIZE];
EXT_RAM_BSS_ATTR static int16_t g_tts_buffer[MAX_WAV_SIZE];

static const char *TAG = "esp-aiot-knob";

#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_STACK_SIZE 8192
#define LVGL_TASK_PRIORITY 4

static void lvgl_tick_inc_callback(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

esp_err_t deepseek_send_request(const char *prompt, char *response, int response_size);

static char *g_response_buffer = NULL;
static int g_response_size = 0;
static int g_response_pos = 0;
static bool g_is_binary_response = false;
static char g_response_content_type[64] = {0};

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            if (strcmp(evt->header_key, "Content-Type") == 0) {
                strncpy(g_response_content_type, evt->header_value, sizeof(g_response_content_type) - 1);
                g_response_content_type[sizeof(g_response_content_type) - 1] = '\0';
            }
            break;
        case HTTP_EVENT_ON_DATA:
            if (g_response_buffer && g_response_pos < g_response_size) {
                int copy_len = MIN(evt->data_len, g_response_size - g_response_pos);
                memcpy(g_response_buffer + g_response_pos, evt->data, copy_len);
                g_response_pos += copy_len;
                if (!g_is_binary_response && g_response_pos < g_response_size) {
                    g_response_buffer[g_response_pos] = '\0';
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t deepseek_send_request(const char *prompt, char *response, int response_size)
{
    if (!wifi_connected) {
        ESP_LOGE(TAG, "WiFi not connected, cannot send request");
        snprintf(response, response_size, "WiFi not connected");
        return ESP_FAIL;
    }
    
    char post_data[512];
    snprintf(post_data, sizeof(post_data), 
             "{\"model\":\"deepseek-v4-flash\",\"messages\":[{\"role\":\"system\",\"content\":\"You are a helpful AI assistant. You MUST respond in English only. Do not use any Chinese characters in your response. Keep your answer concise and friendly.\"},{\"role\":\"user\",\"content\":\"%s\"}],\"stream\":false}", 
             prompt);
    
    ESP_LOGI(TAG, "Post data: %s", post_data);
    
    g_response_buffer = response;
    g_response_size = response_size;
    g_response_pos = 0;
    response[0] = '\0';
    
    esp_http_client_config_t config = {
        .url = DEEPSEEK_API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .event_handler = http_event_handler,
        .skip_cert_common_name_check = true,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        snprintf(response, response_size, "HTTP init fail");
        return ESP_FAIL;
    }
    
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", deepseek_api_key);
    esp_http_client_set_header(client, "Authorization", auth_header);
    
    ESP_LOGI(TAG, "Sending POST request to: %s", DEEPSEEK_API_URL);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        snprintf(response, response_size, "HTTP error: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Status Code: %d", status_code);
    
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed, status code: %d", status_code);
        snprintf(response, response_size, "HTTP error: %d", status_code);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Response length: %d", g_response_pos);
    ESP_LOGI(TAG, "Raw response: %s", response);
    
    char *json_start = strstr(response, "\"content\":\"");
    if (json_start) {
        json_start += 11;
        char *json_end = strstr(json_start, "\"");
        if (json_end) {
            *json_end = '\0';
            strncpy(response, json_start, response_size - 1);
            response[response_size - 1] = '\0';
        }
    }
    
    esp_http_client_cleanup(client);
    return ESP_OK;
}

static esp_err_t baidu_get_access_token(void)
{
    if (baidu_access_token[0] != '\0' && esp_timer_get_time() / 1000 < baidu_token_expire_time) {
        return ESP_OK;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("baidu", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        size_t len = sizeof(baidu_access_token);
        ret = nvs_get_str(nvs_handle, "access_token", baidu_access_token, &len);
        if (ret == ESP_OK && len > 0 && baidu_access_token[0] != '\0') {
            ret = nvs_get_u32(nvs_handle, "expire_time", &baidu_token_expire_time);
            if (ret == ESP_OK && esp_timer_get_time() / 1000 < baidu_token_expire_time) {
                nvs_close(nvs_handle);
                ESP_LOGI(TAG, "Baidu access token loaded from NVS");
                return ESP_OK;
            }
        }
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Getting Baidu access token...");

    char *response_buffer = heap_caps_malloc(2048, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!response_buffer) {
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        return ESP_FAIL;
    }

    char url[512];
    snprintf(url, sizeof(url), 
             "http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s",
             BAIDU_API_KEY, BAIDU_SECRET_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for Baidu token");
        heap_caps_free(response_buffer);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Baidu token request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        heap_caps_free(response_buffer);
        return err;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Baidu token request failed, status: %d", status_code);
        esp_http_client_cleanup(client);
        heap_caps_free(response_buffer);
        return ESP_FAIL;
    }

    int content_length = esp_http_client_get_content_length(client);
    if (content_length > 2048) content_length = 2048;
    
    int read_len = esp_http_client_read(client, response_buffer, content_length);
    response_buffer[read_len] = '\0';

    ESP_LOGI(TAG, "Baidu token response: %s", response_buffer);

    char *token_start = strstr(response_buffer, "\"access_token\":\"");
    if (token_start) {
        token_start += 16;
        char *token_end = strstr(token_start, "\"");
        if (token_end) {
            *token_end = '\0';
            
            char *expire_start = strstr(response_buffer, "\"expires_in\":");
            if (expire_start) {
                expire_start += 14;
                int expires_in = atoi(expire_start);
                baidu_token_expire_time = (esp_timer_get_time() / 1000) + expires_in - 60;
            }
            
            strncpy(baidu_access_token, token_start, sizeof(baidu_access_token) - 1);
            baidu_access_token[sizeof(baidu_access_token) - 1] = '\0';
            
            ret = nvs_open("baidu", NVS_READWRITE, &nvs_handle);
            if (ret == ESP_OK) {
                nvs_set_str(nvs_handle, "access_token", baidu_access_token);
                nvs_set_u32(nvs_handle, "expire_time", baidu_token_expire_time);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
            }
            
            ESP_LOGI(TAG, "Baidu access token obtained: %s", baidu_access_token);
            esp_http_client_cleanup(client);
            heap_caps_free(response_buffer);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "Failed to parse Baidu access token");
    esp_http_client_cleanup(client);
    heap_caps_free(response_buffer);
    return ESP_FAIL;
}

static int wav_header_size = 0;

static void wav_write_header(int16_t *buffer, int sample_rate, int channels, int bits_per_sample, int data_size)
{
    uint8_t *buf = (uint8_t *)buffer;
    int offset = 0;
    
    buf[offset++] = 'R'; buf[offset++] = 'I'; buf[offset++] = 'F'; buf[offset++] = 'F';
    uint32_t file_size = 36 + data_size;
    buf[offset++] = file_size & 0xFF; buf[offset++] = (file_size >> 8) & 0xFF;
    buf[offset++] = (file_size >> 16) & 0xFF; buf[offset++] = (file_size >> 24) & 0xFF;
    
    buf[offset++] = 'W'; buf[offset++] = 'A'; buf[offset++] = 'V'; buf[offset++] = 'E';
    
    buf[offset++] = 'f'; buf[offset++] = 'm'; buf[offset++] = 't'; buf[offset++] = ' ';
    uint32_t fmt_size = 16;
    buf[offset++] = fmt_size & 0xFF; buf[offset++] = (fmt_size >> 8) & 0xFF;
    buf[offset++] = (fmt_size >> 16) & 0xFF; buf[offset++] = (fmt_size >> 24) & 0xFF;
    
    uint16_t fmt_tag = 1;
    buf[offset++] = fmt_tag & 0xFF; buf[offset++] = (fmt_tag >> 8) & 0xFF;
    
    uint16_t num_channels = channels;
    buf[offset++] = num_channels & 0xFF; buf[offset++] = (num_channels >> 8) & 0xFF;
    
    buf[offset++] = sample_rate & 0xFF; buf[offset++] = (sample_rate >> 8) & 0xFF;
    buf[offset++] = (sample_rate >> 16) & 0xFF; buf[offset++] = (sample_rate >> 24) & 0xFF;
    
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    buf[offset++] = byte_rate & 0xFF; buf[offset++] = (byte_rate >> 8) & 0xFF;
    buf[offset++] = (byte_rate >> 16) & 0xFF; buf[offset++] = (byte_rate >> 24) & 0xFF;
    
    uint16_t block_align = channels * bits_per_sample / 8;
    buf[offset++] = block_align & 0xFF; buf[offset++] = (block_align >> 8) & 0xFF;
    
    uint16_t bits = bits_per_sample;
    buf[offset++] = bits & 0xFF; buf[offset++] = (bits >> 8) & 0xFF;
    
    buf[offset++] = 'd'; buf[offset++] = 'a'; buf[offset++] = 't'; buf[offset++] = 'a';
    buf[offset++] = data_size & 0xFF; buf[offset++] = (data_size >> 8) & 0xFF;
    buf[offset++] = (data_size >> 16) & 0xFF; buf[offset++] = (data_size >> 24) & 0xFF;
    
    wav_header_size = offset;
}

static int base64_encode(const unsigned char *input, int input_len, char *output, int output_len)
{
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char three_bytes[3];
    unsigned char four_bytes[4];

    while (input_len--) {
        three_bytes[i++] = *(input++);
        if (i == 3) {
            four_bytes[0] = (three_bytes[0] & 0xfc) >> 2;
            four_bytes[1] = ((three_bytes[0] & 0x03) << 4) + ((three_bytes[1] & 0xf0) >> 4);
            four_bytes[2] = ((three_bytes[1] & 0x0f) << 2) + ((three_bytes[2] & 0xc0) >> 6);
            four_bytes[3] = three_bytes[2] & 0x3f;

            for (i = 0; (i < 4) && (j < output_len); i++) {
                output[j++] = base64_chars[four_bytes[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int k = i; k < 3; k++) {
            three_bytes[k] = '\0';
        }

        four_bytes[0] = (three_bytes[0] & 0xfc) >> 2;
        four_bytes[1] = ((three_bytes[0] & 0x03) << 4) + ((three_bytes[1] & 0xf0) >> 4);
        four_bytes[2] = ((three_bytes[1] & 0x0f) << 2) + ((three_bytes[2] & 0xc0) >> 6);
        four_bytes[3] = three_bytes[2] & 0x3f;

        for (int k = 0; (k < i + 1) && (j < output_len); k++) {
            output[j++] = base64_chars[four_bytes[k]];
        }

        while ((i++ < 3) && (j < output_len)) {
            output[j++] = '=';
        }
    }

    output[j] = '\0';
    return j;
}

esp_err_t baidu_speech_recognize(int16_t *audio_data, int data_len, char *result, int result_size)
{
    if (!wifi_connected) {
        ESP_LOGE(TAG, "WiFi not connected");
        snprintf(result, result_size, "WiFi not connected");
        return ESP_FAIL;
    }

    esp_err_t ret = baidu_get_access_token();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Baidu access token");
        snprintf(result, result_size, "Token error");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Using test audio: 16k.pcm (%d bytes, %d samples)", test_audio_len, test_audio_samples);
    audio_data = (int16_t *)test_audio_data;
    data_len = test_audio_samples;

    int audio_bytes_len = data_len * sizeof(int16_t);
    ESP_LOGI(TAG, "Audio data length: %d bytes, %d samples", audio_bytes_len, data_len);
    ESP_LOGI(TAG, "Token: %s", baidu_access_token);

    int base64_len = (audio_bytes_len + 2) / 3 * 4;
    unsigned char *audioDataBase64 = (unsigned char *)heap_caps_malloc(base64_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!audioDataBase64) {
        ESP_LOGE(TAG, "Failed to allocate memory for audioDataBase64");
        snprintf(result, result_size, "Memory error");
        return ESP_FAIL;
    }

    int json_overhead = 512;
    int data_json_len = base64_len + json_overhead;
    char *data_json = (char *)heap_caps_malloc(data_json_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!data_json) {
        ESP_LOGE(TAG, "Failed to allocate memory for data_json");
        heap_caps_free(audioDataBase64);
        snprintf(result, result_size, "Memory error");
        return ESP_FAIL;
    }

    int encoded_len = base64_encode((const unsigned char *)audio_data, audio_bytes_len, (char *)audioDataBase64, base64_len);
    audioDataBase64[encoded_len] = '\0';
    ESP_LOGI(TAG, "Base64 encoded length: %d", encoded_len);

    memset(data_json, '\0', data_json_len + 1);
    strcat(data_json, "{");
    strcat(data_json, "\"format\":\"pcm\",");
    strcat(data_json, "\"rate\":16000,");
    strcat(data_json, "\"dev_pid\":1537,");
    strcat(data_json, "\"channel\":1,");
    strcat(data_json, "\"cuid\":\"57722200\",");
    strcat(data_json, "\"token\":\"");
    strcat(data_json, baidu_access_token);
    strcat(data_json, "\",");
    sprintf(data_json + strlen(data_json), "\"len\":%d,", audio_bytes_len);
    strcat(data_json, "\"speech\":\"");
    strcat(data_json, (const char *)audioDataBase64);
    strcat(data_json, "\"");
    strcat(data_json, "}");

    ESP_LOGI(TAG, "JSON data length: %d", (int)strlen(data_json));

    g_response_buffer = result;
    g_response_size = result_size;
    g_response_pos = 0;
    result[0] = '\0';

    esp_http_client_config_t config = {
        .url = BAIDU_ASR_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for ASR");
        heap_caps_free(audioDataBase64);
        heap_caps_free(data_json);
        snprintf(result, result_size, "HTTP init fail");
        return ESP_FAIL;
    }

    esp_http_client_set_post_field(client, data_json, strlen(data_json));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");

    ESP_LOGI(TAG, "Sending ASR request...");
    ESP_LOGI(TAG, "URL: %s", BAIDU_ASR_URL);

    ret = esp_http_client_perform(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ASR request failed: %s", esp_err_to_name(ret));
        snprintf(result, result_size, "ASR error: %s", esp_err_to_name(ret));
        esp_http_client_cleanup(client);
        heap_caps_free(audioDataBase64);
        heap_caps_free(data_json);
        return ret;
    }

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "ASR Status: %d", status_code);

    if (status_code != 200) {
        ESP_LOGE(TAG, "ASR failed, status: %d", status_code);
        snprintf(result, result_size, "ASR error: %d", status_code);
        esp_http_client_cleanup(client);
        heap_caps_free(audioDataBase64);
        heap_caps_free(data_json);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ASR response: %s", result);

    char *result_start = strstr(result, "\"result\":[\"");
    if (result_start) {
        result_start += 11;
        char *result_end = strstr(result_start, "\"]");
        if (result_end) {
            *result_end = '\0';
            strncpy(result, result_start, result_size - 1);
            result[result_size - 1] = '\0';
            ESP_LOGI(TAG, "Recognized text: %s", result);
        }
    } else {
        char *err_msg = strstr(result, "\"err_msg\":\"");
        if (err_msg) {
            err_msg += 11;
            char *err_end = strstr(err_msg, "\"");
            if (err_end) *err_end = '\0';
            snprintf(result, result_size, "ASR error: %s", err_msg);
        } else {
            snprintf(result, result_size, "ASR failed");
        }
        ret = ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    heap_caps_free(audioDataBase64);
    heap_caps_free(data_json);
    return ret;
}

#define BAIDU_TTS_URL "http://tsn.baidu.com/text2audio"

static void url_encode(const char *src, char *dst, int dst_size)
{
    int i, j = 0;
    const char hex[] = "0123456789ABCDEF";
    
    for (i = 0; src[i] && j < dst_size - 1; i++) {
        if ((src[i] >= 'A' && src[i] <= 'Z') ||
            (src[i] >= 'a' && src[i] <= 'z') ||
            (src[i] >= '0' && src[i] <= '9') ||
            src[i] == '-' || src[i] == '_' || src[i] == '.' || src[i] == '~') {
            dst[j++] = src[i];
        } else if (src[i] == ' ') {
            dst[j++] = '+';
        } else {
            if (j + 3 >= dst_size) break;
            dst[j++] = '%';
            dst[j++] = hex[(unsigned char)src[i] >> 4];
            dst[j++] = hex[(unsigned char)src[i] & 0x0F];
        }
    }
    dst[j] = '\0';
}

esp_err_t baidu_tts_synthesize(const char *text, int16_t *buffer, size_t *len)
{
    if (!wifi_connected) {
        ESP_LOGE(TAG, "WiFi not connected");
        *len = 0;
        return ESP_FAIL;
    }

    esp_err_t ret = baidu_get_access_token();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Baidu access token");
        *len = 0;
        return ESP_FAIL;
    }

    char encoded_text[1536];
    url_encode(text, encoded_text, sizeof(encoded_text));

    char post_data[2048];
    snprintf(post_data, sizeof(post_data), 
             "tex=%s&tok=%s&cuid=%s&ctp=1&lan=zh&spd=5&pit=5&vol=5&per=1&aue=6",
             encoded_text, baidu_access_token, BAIDU_API_KEY);

    ESP_LOGI(TAG, "TTS POST data: %s", post_data);

    g_response_buffer = (char *)g_tts_buffer;
    g_response_size = sizeof(g_tts_buffer);
    g_response_pos = 0;
    g_is_binary_response = true;
    g_response_content_type[0] = '\0';

    esp_http_client_config_t config = {
        .url = BAIDU_TTS_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for TTS");
        *len = 0;
        g_is_binary_response = false;
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "cuid", "esp32");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    ret = esp_http_client_perform(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TTS request failed: %s", esp_err_to_name(ret));
        *len = 0;
        esp_http_client_cleanup(client);
        g_is_binary_response = false;
        return ret;
    }

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "TTS Status: %d", status_code);

    if (status_code != 200) {
        ESP_LOGE(TAG, "TTS failed, status: %d", status_code);
        ESP_LOGI(TAG, "TTS response content: %s", (char *)g_tts_buffer);
        *len = 0;
        esp_http_client_cleanup(client);
        g_is_binary_response = false;
        return ESP_FAIL;
    }

    if (g_response_content_type[0] != '\0' && strstr(g_response_content_type, "audio")) {
        *len = g_response_pos / sizeof(int16_t);
        ESP_LOGI(TAG, "TTS audio received: %d bytes (%d samples), Content-Type: %s", g_response_pos, *len, g_response_content_type);
        
        if (g_response_pos > *len * sizeof(int16_t)) {
            g_response_pos = *len * sizeof(int16_t);
        }
        memcpy(buffer, g_tts_buffer, g_response_pos);
    } else {
        ESP_LOGE(TAG, "TTS response is not audio, Content-Type: %s", g_response_content_type[0] != '\0' ? g_response_content_type : "NULL");
        ESP_LOGI(TAG, "TTS response content (first 200 bytes): ");
        for (int i = 0; i < MIN(g_response_pos, 200); i++) {
            ESP_LOGI(TAG, "%02X ", (unsigned char)g_response_buffer[i]);
        }
        *len = 0;
        ret = ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    g_is_binary_response = false;
    return ret;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        strcpy(wifi_ip, "0.0.0.0");
        extern void lvgl_ui_update_wifi(void);
        lvgl_ui_update_wifi();
        static int s_retry_num = 0;
        if (s_retry_num < CONFIG_WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", s_retry_num, CONFIG_WIFI_MAXIMUM_RETRY);
        } else {
            ESP_LOGI(TAG, "WiFi connection failed after %d retries", CONFIG_WIFI_MAXIMUM_RETRY);
            s_retry_num = 0;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        sprintf(wifi_ip, IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        ESP_LOGI(TAG, "WiFi connected, IP: %s", wifi_ip);
        
        extern void lvgl_ui_update_wifi(void);
        lvgl_ui_update_wifi();
    }
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized, not connected");
}

void wifi_connect(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "WiFi connecting to %s...", wifi_ssid);
}

#define LED_PIN 38

void led_blink_task(void *arg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void lvgl_task_handler(void *arg)
{
    ESP_LOGI(TAG, "LVGL task starting on CPU %d", xPortGetCoreID());
    
    lv_init();
    
    lvgl_display_init();
    
    lv_display_t *disp = lv_display_create(240, 280);
    lv_display_set_flush_cb(disp, (lv_display_flush_cb_t)lvgl_display_flush);
    
    lv_color_t *buf1 = heap_caps_malloc(240 * 20 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buf2 = heap_caps_malloc(240 * 20 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) buf1 = heap_caps_malloc(240 * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    if (!buf2) buf2 = heap_caps_malloc(240 * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    lv_display_set_buffers(disp, buf1, buf2, 240 * 20 * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lvgl_input_init();
    
    ESP_LOGI(TAG, "LVGL initialized, creating UI...");
    
    lvgl_ui_init();
    
    ppt_controller_init(NULL, 0);
    
    ESP_LOGI(TAG, "LVGL UI loaded");
    
    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        time_till_next = MAX(time_till_next, 1);
        time_till_next = MIN(time_till_next, 500);
        usleep(time_till_next * 1000);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "ESP32-S3 Test Program");
    ESP_LOGI(TAG, "Team: Only 3 hours left");
    ESP_LOGI(TAG, "Team ID: 28095");
    ESP_LOGI(TAG, "==========================================");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    wifi_init_sta();
    ESP_LOGI(TAG, "WiFi initialized");

    audio_i2s_init();
    vTaskDelay(pdMS_TO_TICKS(50));
    es8311_codec_init();
    voice_task_start();
    ESP_LOGI(TAG, "Audio and voice initialized");

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_inc_callback,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    ESP_LOGI(TAG, "LVGL tick timer started");

    xTaskCreatePinnedToCore(lvgl_task_handler, "lvgl_task", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(led_blink_task, "led_task", 4096, NULL, 2, NULL, 0);
    ESP_LOGI(TAG, "Tasks created");
}