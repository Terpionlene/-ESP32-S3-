#pragma once
#include <stdint.h>

void es8311_codec_init(void);
void audio_i2s_init(void);
void audio_play_buffer(int16_t *buffer, size_t len);
void audio_start_recording(void);
void audio_stop_recording(int16_t *buffer, size_t *len);
void audio_read_samples(int16_t *buffer, size_t max_samples, size_t *read_samples);
void audio_set_volume(int volume);
void audio_set_mute(bool mute);
