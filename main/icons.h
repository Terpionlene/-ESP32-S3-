#pragma once

#include <stdint.h>
#include "lvgl/lvgl.h"

#define ICON_SIZE 32

extern uint16_t icon_star[ICON_SIZE * ICON_SIZE];
extern uint16_t icon_wifi[ICON_SIZE * ICON_SIZE];
extern uint16_t icon_sun[ICON_SIZE * ICON_SIZE];
extern uint16_t icon_settings[ICON_SIZE * ICON_SIZE];
extern uint16_t icon_mic[ICON_SIZE * ICON_SIZE];
extern uint16_t icon_chat[ICON_SIZE * ICON_SIZE];

extern const lv_image_dsc_t img_dsc_star;
extern const lv_image_dsc_t img_dsc_wifi;
extern const lv_image_dsc_t img_dsc_sun;
extern const lv_image_dsc_t img_dsc_settings;
extern const lv_image_dsc_t img_dsc_mic;
extern const lv_image_dsc_t img_dsc_chat;

void icons_init(void);
