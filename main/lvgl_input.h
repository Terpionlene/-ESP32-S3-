#ifndef LVGL_INPUT_H
#define LVGL_INPUT_H

#include "lvgl/lvgl.h"

typedef void (*ppt_exit_cb_t)(void);

void lvgl_input_init(void);
void lvgl_input_read(lv_indev_t *indev, lv_indev_data_t *data);
void lvgl_input_deinit(void);
lv_group_t *lvgl_input_get_group(void);
lv_indev_t *lvgl_input_get_indev(void);
void lvgl_input_set_ppt_mode(bool enable);
void lvgl_input_set_ppt_exit_callback(ppt_exit_cb_t cb);

#endif