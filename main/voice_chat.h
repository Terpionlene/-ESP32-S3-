#pragma once
#include "esp_err.h"

typedef enum {
    VOICE_STATE_IDLE,
    VOICE_STATE_RECORDING,
    VOICE_STATE_SENDING,
    VOICE_STATE_PLAYING,
} voice_state_t;

extern volatile voice_state_t voice_state;
extern char voice_transcript[512];

void voice_record_start(void);
void voice_record_stop(void);
void voice_send_request(void);
void voice_task_start(void);
voice_state_t voice_get_state(void);
const char *voice_get_response(void);
const char *voice_get_transcript(void);
void voice_set_response_callback(void (*cb)(const char *response));
