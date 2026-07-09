#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "esp_lcd_panel_ops.h"
#include "lvgl/lvgl.h"

void lvgl_display_init(void);
void lvgl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void lvgl_display_deinit(void);
esp_lcd_panel_handle_t lvgl_get_panel_handle(void);
void lvgl_display_set_brightness(int brightness);

#endif