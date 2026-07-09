#include "voice_chat.h"
#include "audio_es8311.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include <stdint.h>
#include <math.h>
#include <string.h>

#define TAG "voice-chat"
#define RECORD_BUFFER_SIZE 16384
#define RECORD_TIME_MS 5000
#define SAMPLE_RATE 16000
#define VOICE_CHAT_TASK_STACK 8192
#define VOICE_CHAT_TASK_PRIORITY 5

static int16_t record_buffer[RECORD_BUFFER_SIZE];
static size_t record_len = 0;
static volatile bool is_recording = false;
volatile voice_state_t voice_state = VOICE_STATE_IDLE;
char voice_transcript[512] = {0};
static char ai_text_response[512] = {0};
EXT_RAM_BSS_ATTR static int16_t speech_buffer[RECORD_BUFFER_SIZE];
static TaskHandle_t record_task_handle = NULL;
static TaskHandle_t voice_chat_task_handle = NULL;
static QueueHandle_t voice_chat_queue = NULL;

typedef void (*voice_chat_callback_t)(const char *response);
static voice_chat_callback_t g_voice_chat_callback = NULL;

esp_err_t deepseek_send_request(const char* prompt, char* response, int response_len);
esp_err_t baidu_speech_recognize(int16_t *audio_data, int data_len, char *result, int result_size);
esp_err_t baidu_tts_synthesize(const char *text, int16_t *buffer, size_t *len);

static void synthesize_speech(const char *text, int16_t *buffer, size_t *len)
{
    *len = 0;
    float speed = 0.06f;
    float freq_base = 250.0f;
    float freq_range = 80.0f;
    
    for (int i = 0; text[i] && *len < RECORD_BUFFER_SIZE; i++) {
        float char_freq = freq_base + ((unsigned char)text[i] % 26) * freq_range / 26;
        int samples_per_char = (int)(SAMPLE_RATE * speed);
        
        for (int j = 0; j < samples_per_char && *len < RECORD_BUFFER_SIZE; j++) {
            float t = j / (float)SAMPLE_RATE;
            float wave = sin(2 * 3.1415926 * char_freq * t) * 0.5 + 
                        sin(2 * 3.1415926 * char_freq * 2 * t) * 0.3;
            float envelope = (j < samples_per_char / 4) ? (j * 4.0f / samples_per_char) :
                             (j > samples_per_char * 3 / 4) ? ((samples_per_char - j) * 4.0f / samples_per_char) : 1.0f;
            buffer[*len] = (int16_t)(wave * envelope * 32767 * 0.4);
            (*len)++;
        }
    }
}

static void record_task(void *arg)
{
    size_t read_samples;
    
    while (1) {
        if (is_recording && record_len < RECORD_BUFFER_SIZE) {
            audio_read_samples(&record_buffer[record_len], 
                            RECORD_BUFFER_SIZE - record_len, 
                            &read_samples);
            if (read_samples > 0) {
                record_len += read_samples;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

typedef enum {
    VOICE_CMD_START,
} voice_cmd_t;

static void voice_chat_task(void *arg)
{
    voice_cmd_t cmd;
    
    while (1) {
        if (xQueueReceive(voice_chat_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            if (cmd == VOICE_CMD_START) {
                ESP_LOGI(TAG, "Voice chat task started");
                
                voice_state = VOICE_STATE_RECORDING;
                is_recording = true;
                record_len = 0;
                ESP_LOGI(TAG, "Start recording...");
                
                vTaskDelay(pdMS_TO_TICKS(RECORD_TIME_MS));
                
                is_recording = false;
                ESP_LOGI(TAG, "Stop recording...");
                voice_state = VOICE_STATE_IDLE;
                ESP_LOGI(TAG, "Recorded %d samples (%d ms)", record_len, (int)(record_len * 1000 / SAMPLE_RATE));
                
                voice_send_request();
            }
        }
    }
}

void voice_record_start(void)
{
    if (is_recording) return;
    ESP_LOGI(TAG, "Start recording requested...");
    voice_cmd_t cmd = VOICE_CMD_START;
    xQueueSend(voice_chat_queue, &cmd, 0);
}

void voice_record_stop(void)
{
    if (!is_recording) return;
    ESP_LOGI(TAG, "Stop recording...");
    is_recording = false;
    ESP_LOGI(TAG, "Recorded %d samples (%d ms)", record_len, (int)(record_len * 1000 / SAMPLE_RATE));
    voice_state = VOICE_STATE_IDLE;
}

void voice_send_request(void)
{
    voice_state = VOICE_STATE_SENDING;
    ESP_LOGI(TAG, "Voice chat processing...");
    
    ai_text_response[0] = '\0';
    voice_transcript[0] = '\0';
    
    if (record_len > 0) {
        int record_ms = (int)(record_len * 1000 / SAMPLE_RATE);
        ESP_LOGI(TAG, "Recording length: %d samples (%d ms)", record_len, record_ms);
        
        if (record_ms < 500) {
            snprintf(ai_text_response, sizeof(ai_text_response), "Recording too short, please try again.");
        } else {
            ESP_LOGI(TAG, "Step 1: Speech to Text (STT)...");
            
            esp_err_t ret = baidu_speech_recognize(record_buffer, record_len, voice_transcript, sizeof(voice_transcript));
            
            if (ret == ESP_OK && voice_transcript[0] != '\0') {
                ESP_LOGI(TAG, "Recognized text: %s", voice_transcript);
                
                ESP_LOGI(TAG, "Step 2: Sending to DeepSeek...");
                
                ret = deepseek_send_request(voice_transcript, ai_text_response, sizeof(ai_text_response));
                
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "DeepSeek request failed");
                    snprintf(ai_text_response, sizeof(ai_text_response), "Sorry, AI service is temporarily unavailable.");
                }
            } else {
                ESP_LOGE(TAG, "STT failed: %s", voice_transcript);
                snprintf(ai_text_response, sizeof(ai_text_response), "Sorry, I didn't catch what you said.");
            }
        }
    } else {
        snprintf(ai_text_response, sizeof(ai_text_response), "No sound detected.");
    }
    
    ESP_LOGI(TAG, "AI response: %s", ai_text_response);
    
    if (g_voice_chat_callback) {
        g_voice_chat_callback(ai_text_response);
    }
    
    voice_state = VOICE_STATE_PLAYING;
    
    ESP_LOGI(TAG, "TTS skipped as requested");
    
    voice_state = VOICE_STATE_IDLE;
}

void voice_task_start(void)
{
    voice_chat_queue = xQueueCreate(1, sizeof(voice_cmd_t));
    xTaskCreatePinnedToCore(record_task, "record_task", 4096, NULL, 5, &record_task_handle, 1);
    xTaskCreatePinnedToCore(voice_chat_task, "voice_chat_task", VOICE_CHAT_TASK_STACK, NULL, VOICE_CHAT_TASK_PRIORITY, &voice_chat_task_handle, 1);
    ESP_LOGI(TAG, "Voice task initialized on CPU 1");
}

voice_state_t voice_get_state(void)
{
    return voice_state;
}

const char *voice_get_response(void)
{
    return ai_text_response;
}

const char *voice_get_transcript(void)
{
    return voice_transcript;
}

void voice_set_response_callback(voice_chat_callback_t cb)
{
    g_voice_chat_callback = cb;
}