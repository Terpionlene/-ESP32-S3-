#include "icons.h"
#include <math.h>
#include "lvgl/lvgl.h"

#define ICON_BG 0x0000
#define ICON_COLOR 0xFFFF

static int draw_star(int x, int y)
{
    int cx = 16, cy = 16;
    float scale = 0.8;
    for (int i = 0; i < 5; i++) {
        float angle1 = (float)(i * 4 * M_PI) / 5 - M_PI / 2;
        float dx = x - cx, dy = y - cy;
        float dist = sqrt(dx * dx + dy * dy);
        float angle = atan2(dy, dx);
        float rel_angle = angle - angle1;
        while (rel_angle < 0) rel_angle += 2 * M_PI;
        while (rel_angle >= 2 * M_PI) rel_angle -= 2 * M_PI;
        
        if (dist <= 10 * scale && rel_angle < (float)(4 * M_PI) / 5) {
            float inner_r = 4 * scale;
            float t = rel_angle / ((float)(4 * M_PI) / 5);
            float r = inner_r + (10 * scale - inner_r) * (t < 0.5 ? 2 * t : 2 * (1 - t));
            if (dist <= r) return 1;
        }
    }
    return 0;
}

static int draw_wifi(int x, int y)
{
    int cx = 16, cy = 18;
    for (int ring = 0; ring < 3; ring++) {
        int r = 3 + ring * 4;
        int y_start = cy - r;
        int y_end = cy;
        if (y >= y_start && y <= y_end) {
            int dx = (int)sqrt((double)(r * r - (y - cy) * (y - cy)));
            if (x >= cx - dx && x <= cx + dx) {
                int border = 2;
                if (abs(x - cx) >= dx - border || abs(y - cy) >= r - border) {
                    return 1;
                }
            }
        }
    }
    if (y >= 25 && y <= 27 && x >= 15 && x <= 17) return 1;
    return 0;
}

static int draw_sun(int x, int y)
{
    int cx = 16, cy = 16;
    int r_core = 6;
    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r_core * r_core) {
        return 1;
    }
    for (int i = 0; i < 12; i++) {
        float angle = (float)(i * 2 * M_PI) / 12;
        int x_ray = cx + (int)(12 * cos(angle));
        int y_ray = cy + (int)(12 * sin(angle));
        int dx = abs(x - x_ray);
        int dy = abs(y - y_ray);
        if (dx <= 1 && dy <= 3) return 1;
    }
    return 0;
}

static int draw_settings(int x, int y)
{
    int cx = 16, cy = 16;
    int r = 10;
    for (int ring = 2; ring <= r; ring += 4) {
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy <= ring * ring && dx * dx + dy * dy >= (ring - 2) * (ring - 2)) {
            return 1;
        }
    }
    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= 4) return 1;
    int dx = x - cx, dy = y - cy;
    float angle = atan2(dy, dx);
    float rel_angle = angle - M_PI / 4;
    while (rel_angle < 0) rel_angle += 2 * M_PI;
    while (rel_angle >= 2 * M_PI) rel_angle -= 2 * M_PI;
    if (rel_angle < M_PI / 6 || rel_angle > (float)(11 * M_PI) / 6) {
        float dist = sqrt(dx * dx + dy * dy);
        if (dist >= 10 && dist <= 14) {
            if (abs(x - (cx + 12)) <= 1) return 1;
        }
    }
    if (y >= 27 && y <= 29 && x >= 15 && x <= 17) return 1;
    return 0;
}

static int draw_mic(int x, int y)
{
    int cx = 16;
    if (y >= 6 && y <= 22) {
        int w = 3 + (y - 6) / 3;
        if (x >= cx - w && x <= cx + w) {
            if (x == cx - w || x == cx + w || y == 6 || y == 22) {
                return 1;
            }
        }
    }
    if (y >= 22 && y <= 28) {
        if (x >= 12 && x <= 20) {
            if (y == 22 || y == 28 || x == 12 || x == 20) return 1;
        }
    }
    if (y >= 10 && y <= 14) {
        if (x >= 20 && x <= 24 && y >= 12 && y <= 14) return 1;
        if (x >= 20 && x <= 22 && y >= 14 && y <= 18) return 1;
        if (x >= 22 && x <= 24 && y >= 14 && y <= 18) return 1;
        if (x >= 20 && x <= 26 && y >= 18 && y <= 20) return 1;
    }
    return 0;
}

static int draw_chat(int x, int y)
{
    int cx = 16, cy = 18;
    int r = 10;
    if (y >= cy - r && y <= cy + r && x >= cx - r && x <= cx + r) {
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy <= r * r) {
            if (!(dy <= -6 && dx >= 6)) {
                return 1;
            }
        }
    }
    if (y >= cy + r - 4 && y <= cy + r && x >= cx + r - 4 && x <= cx + r) {
        if ((x - (cx + r - 2)) * (x - (cx + r - 2)) + (y - (cy + r - 2)) * (y - (cy + r - 2)) <= 4) {
            return 1;
        }
    }
    return 0;
}

uint16_t icon_star[ICON_SIZE * ICON_SIZE];
uint16_t icon_wifi[ICON_SIZE * ICON_SIZE];
uint16_t icon_sun[ICON_SIZE * ICON_SIZE];
uint16_t icon_settings[ICON_SIZE * ICON_SIZE];
uint16_t icon_mic[ICON_SIZE * ICON_SIZE];
uint16_t icon_chat[ICON_SIZE * ICON_SIZE];

const lv_image_dsc_t img_dsc_star = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_star,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

const lv_image_dsc_t img_dsc_wifi = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_wifi,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

const lv_image_dsc_t img_dsc_sun = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_sun,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

const lv_image_dsc_t img_dsc_settings = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_settings,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

const lv_image_dsc_t img_dsc_mic = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_mic,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

const lv_image_dsc_t img_dsc_chat = {
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = ICON_SIZE,
    .header.h = ICON_SIZE,
    .data = (const uint8_t *)icon_chat,
    .data_size = ICON_SIZE * ICON_SIZE * 2
};

void icons_init(void)
{
    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            icon_star[y * ICON_SIZE + x] = draw_star(x, y) ? ICON_COLOR : ICON_BG;
            icon_wifi[y * ICON_SIZE + x] = draw_wifi(x, y) ? ICON_COLOR : ICON_BG;
            icon_sun[y * ICON_SIZE + x] = draw_sun(x, y) ? ICON_COLOR : ICON_BG;
            icon_settings[y * ICON_SIZE + x] = draw_settings(x, y) ? ICON_COLOR : ICON_BG;
            icon_mic[y * ICON_SIZE + x] = draw_mic(x, y) ? ICON_COLOR : ICON_BG;
            icon_chat[y * ICON_SIZE + x] = draw_chat(x, y) ? ICON_COLOR : ICON_BG;
        }
    }
}
