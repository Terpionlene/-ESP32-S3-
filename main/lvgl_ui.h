#ifndef LVGL_UI_H
#define LVGL_UI_H

#include "lvgl/lvgl.h"

extern lv_obj_t *screen_main;

void lvgl_ui_init(void);
void lvgl_ui_create_main_screen(void);
void lvgl_ui_create_settings_screen(void);
void lvgl_ui_create_smart_screen(void);
void lvgl_ui_create_chat_screen(void);
void lvgl_ui_create_sound_screen(void);
void lvgl_ui_create_brightness_screen(void);
void lvgl_ui_create_wifi_screen(void);
void lvgl_ui_update_wifi(void);
void lvgl_ui_append_chat_message(const char *msg);

#endif