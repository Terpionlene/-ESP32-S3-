#include <string.h>
#include "lvgl/lvgl.h"
#include "audio_es8311.h"
#include "voice_chat.h"
#include "lvgl_input.h"
#include "lvgl_display.h"
#include "esp_wifi.h"
#include "ppt_controller.h"

extern int sound_volume;
extern bool sound_mute;
extern int lcd_brightness;
extern bool wifi_connected;
extern char wifi_ip[16];

lv_obj_t *screen_main;
static lv_obj_t *screen_settings;
static lv_obj_t *screen_smart;
static lv_obj_t *screen_chat;
static lv_obj_t *screen_sound;
static lv_obj_t *screen_brightness;
static lv_obj_t *screen_wifi;

static lv_obj_t *chat_textarea;
static lv_obj_t *slider_volume;
static lv_obj_t *slider_bright;
static lv_obj_t *sw_mute;
static lv_obj_t *label_wifi_status;
static lv_obj_t *label_wifi_ip;
static lv_obj_t *btn_wifi_connect;
static lv_obj_t *label_wifi_connect;

static lv_obj_t *screen_ppt_settings;
static lv_obj_t *screen_ppt_control;
static lv_obj_t *label_ppt_ip;
static lv_obj_t *label_ppt_port;
static lv_obj_t *label_ppt_status;
static uint8_t ppt_ip_octets[4];
static int ppt_port_val;
static int ppt_edit_field;

static lv_obj_t *btn_main_first;
static lv_obj_t *btn_settings_first;
static lv_obj_t *btn_smart_first;
static lv_obj_t *btn_chat_first;
static lv_obj_t *btn_sound_first;
static lv_obj_t *btn_bright_first;
static lv_obj_t *btn_wifi_first;
static lv_obj_t *btn_ppt_first;

static lv_group_t *group_main;
static lv_group_t *group_settings;
static lv_group_t *group_smart;
static lv_group_t *group_chat;
static lv_group_t *group_sound;
static lv_group_t *group_brightness;
static lv_group_t *group_wifi;
static lv_group_t *group_ppt_settings;

static lv_indev_t *s_indev = NULL;

static void lvgl_ui_show_main(lv_event_t *e);
static void lvgl_ui_show_settings(lv_event_t *e);
static void lvgl_ui_show_smart(lv_event_t *e);
static void lvgl_ui_show_chat(lv_event_t *e);
static void lvgl_ui_show_sound(lv_event_t *e);
static void lvgl_ui_show_brightness(lv_event_t *e);
static void lvgl_ui_show_wifi(lv_event_t *e);
static void lvgl_ui_volume_changed(lv_event_t *e);
static void lvgl_ui_mute_changed(lv_event_t *e);
static void lvgl_ui_brightness_changed(lv_event_t *e);
static void lvgl_ui_start_voice_chat(lv_event_t *e);
static void lvgl_ui_wifi_connect(lv_event_t *e);
static void lvgl_ui_ppt_control(lv_event_t *e);
static void lvgl_ui_ppt_exit_cb(void);
static void lvgl_ui_aircon_control(lv_event_t *e);
static void lvgl_ui_show_ppt_settings(lv_event_t *e);
static void lvgl_ui_ppt_settings_back(lv_event_t *e);
static void lvgl_ui_ppt_field_event(lv_event_t *e);
static void lvgl_ui_create_ppt_control_screen(void);
void lvgl_ui_ppt_update_status(const char *status);
void lvgl_ui_update_wifi(void);
void lvgl_ui_append_chat_message(const char *msg);

void lvgl_ui_create_main_screen(void)
{
    group_main = lv_group_create();
    
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_main);
    lv_label_set_text(title, "AI ASSISTANT");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *btn_chat = lv_button_create(screen_main);
    lv_obj_set_size(btn_chat, 220, 50);
    lv_obj_set_style_bg_color(btn_chat, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_chat, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_border_width(btn_chat, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_chat, 6, LV_PART_MAIN);
    lv_obj_align(btn_chat, LV_ALIGN_TOP_MID, 0, 55);
    lv_group_add_obj(group_main, btn_chat);
    btn_main_first = btn_chat;

    lv_obj_t *label_chat = lv_label_create(btn_chat);
    lv_label_set_text(label_chat, LV_SYMBOL_ENVELOPE " AI CHAT");
    lv_obj_set_style_text_color(label_chat, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_chat, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_chat, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_smart = lv_button_create(screen_main);
    lv_obj_set_size(btn_smart, 220, 50);
    lv_obj_set_style_bg_color(btn_smart, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_smart, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_border_width(btn_smart, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_smart, 6, LV_PART_MAIN);
    lv_obj_align(btn_smart, LV_ALIGN_TOP_MID, 0, 110);
    lv_group_add_obj(group_main, btn_smart);

    lv_obj_t *label_smart = lv_label_create(btn_smart);
    lv_label_set_text(label_smart, LV_SYMBOL_BLUETOOTH " SMART CTRL");
    lv_obj_set_style_text_color(label_smart, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_smart, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_smart, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_settings = lv_button_create(screen_main);
    lv_obj_set_size(btn_settings, 220, 50);
    lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_border_width(btn_settings, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_settings, 6, LV_PART_MAIN);
    lv_obj_align(btn_settings, LV_ALIGN_TOP_MID, 0, 165);
    lv_group_add_obj(group_main, btn_settings);

    lv_obj_t *label_settings = lv_label_create(btn_settings);
    lv_label_set_text(label_settings, LV_SYMBOL_SETTINGS " SETTINGS");
    lv_obj_set_style_text_color(label_settings, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_settings, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_settings, LV_ALIGN_CENTER, 0, 0);

    lv_group_set_wrap(group_main, true);

    lv_obj_add_event_cb(btn_chat, lvgl_ui_show_chat, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_smart, lvgl_ui_show_smart, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_settings, lvgl_ui_show_settings, LV_EVENT_CLICKED, NULL);
}

void lvgl_ui_create_settings_screen(void)
{
    group_settings = lv_group_create();
    
    screen_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_settings);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *btn_back = lv_button_create(screen_settings);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_settings, btn_back);
    btn_settings_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);
    
    lv_obj_t *btn_wifi = lv_button_create(screen_settings);
    lv_obj_set_size(btn_wifi, 220, 45);
    lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_wifi, 4, LV_PART_MAIN);
    lv_obj_align(btn_wifi, LV_ALIGN_TOP_MID, 0, 60);
    lv_group_add_obj(group_settings, btn_wifi);

    lv_obj_t *label_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(label_wifi, LV_SYMBOL_WIFI " WIFI");
    lv_obj_set_style_text_color(label_wifi, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_wifi, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_sound = lv_button_create(screen_settings);
    lv_obj_set_size(btn_sound, 220, 45);
    lv_obj_set_style_bg_color(btn_sound, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_sound, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_sound, 4, LV_PART_MAIN);
    lv_obj_align(btn_sound, LV_ALIGN_TOP_MID, 0, 110);
    lv_group_add_obj(group_settings, btn_sound);

    lv_obj_t *label_sound = lv_label_create(btn_sound);
    lv_label_set_text(label_sound, LV_SYMBOL_VOLUME_MID " VOLUME");
    lv_obj_set_style_text_color(label_sound, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_sound, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_sound, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_brightness = lv_button_create(screen_settings);
    lv_obj_set_size(btn_brightness, 220, 45);
    lv_obj_set_style_bg_color(btn_brightness, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_brightness, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_brightness, 4, LV_PART_MAIN);
    lv_obj_align(btn_brightness, LV_ALIGN_TOP_MID, 0, 160);
    lv_group_add_obj(group_settings, btn_brightness);

    lv_obj_t *label_brightness = lv_label_create(btn_brightness);
    lv_label_set_text(label_brightness, LV_SYMBOL_TINT " BRIGHTNESS");
    lv_obj_set_style_text_color(label_brightness, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_brightness, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_brightness, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_ppt_set = lv_button_create(screen_settings);
    lv_obj_set_size(btn_ppt_set, 220, 45);
    lv_obj_set_style_bg_color(btn_ppt_set, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ppt_set, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ppt_set, 4, LV_PART_MAIN);
    lv_obj_align(btn_ppt_set, LV_ALIGN_TOP_MID, 0, 210);
    lv_group_add_obj(group_settings, btn_ppt_set);

    lv_obj_t *label_ppt_set = lv_label_create(btn_ppt_set);
    lv_label_set_text(label_ppt_set, LV_SYMBOL_SETTINGS " PPT SERVER");
    lv_obj_set_style_text_color(label_ppt_set, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ppt_set, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_ppt_set, LV_ALIGN_CENTER, 0, 0);

    lv_group_set_wrap(group_settings, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_main, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_wifi, lvgl_ui_show_wifi, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_sound, lvgl_ui_show_sound, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_brightness, lvgl_ui_show_brightness, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_ppt_set, lvgl_ui_show_ppt_settings, LV_EVENT_CLICKED, NULL);
}

void lvgl_ui_create_smart_screen(void)
{
    group_smart = lv_group_create();
    
    screen_smart = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_smart, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_smart);
    lv_label_set_text(title, "SMART CTRL");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *btn_back = lv_button_create(screen_smart);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_smart, btn_back);
    btn_smart_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *btn_ppt = lv_button_create(screen_smart);
    lv_obj_set_size(btn_ppt, 220, 50);
    lv_obj_set_style_bg_color(btn_ppt, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ppt, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ppt, 6, LV_PART_MAIN);
    lv_obj_align(btn_ppt, LV_ALIGN_TOP_MID, 0, 60);
    lv_group_add_obj(group_smart, btn_ppt);

    lv_obj_t *label_ppt = lv_label_create(btn_ppt);
    lv_label_set_text(label_ppt, LV_SYMBOL_BLUETOOTH " PPT CONTROL");
    lv_obj_set_style_text_color(label_ppt, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ppt, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_ppt, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_ac = lv_button_create(screen_smart);
    lv_obj_set_size(btn_ac, 220, 50);
    lv_obj_set_style_bg_color(btn_ac, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ac, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ac, 6, LV_PART_MAIN);
    lv_obj_align(btn_ac, LV_ALIGN_TOP_MID, 0, 120);
    lv_group_add_obj(group_smart, btn_ac);

    lv_obj_t *label_ac = lv_label_create(btn_ac);
    lv_label_set_text(label_ac, LV_SYMBOL_BARS " AIR CON");
    lv_obj_set_style_text_color(label_ac, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ac, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_ac, LV_ALIGN_CENTER, 0, 0);

    lv_group_set_wrap(group_smart, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_main, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_ppt, lvgl_ui_ppt_control, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_ac, lvgl_ui_aircon_control, LV_EVENT_CLICKED, NULL);
}

void lvgl_ui_create_chat_screen(void)
{
    group_chat = lv_group_create();
    
    screen_chat = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_chat, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_chat);
    lv_label_set_text(title, "AI CHAT");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn_back = lv_button_create(screen_chat);
    lv_obj_set_size(btn_back, 60, 40);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_chat, btn_back);
    btn_chat_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    chat_textarea = lv_textarea_create(screen_chat);
    lv_obj_set_size(chat_textarea, 220, 120);
    lv_obj_align(chat_textarea, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_color(chat_textarea, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_text_color(chat_textarea, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(chat_textarea, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_add_flag(chat_textarea, LV_OBJ_FLAG_SCROLLABLE);


    lv_obj_t *btn_record = lv_button_create(screen_chat);
    lv_obj_set_size(btn_record, 70, 70);
    lv_obj_set_style_bg_color(btn_record, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_record, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_record, 35, LV_PART_MAIN);
    lv_obj_align(btn_record, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_group_add_obj(group_chat, btn_record);

    lv_obj_t *label_record = lv_label_create(btn_record);
    lv_label_set_text(label_record, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(label_record, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_record, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_record, LV_ALIGN_CENTER, 0, 0);

    lv_group_set_wrap(group_chat, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_main, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_record, lvgl_ui_start_voice_chat, LV_EVENT_CLICKED, NULL);
}

void lvgl_ui_create_sound_screen(void)
{
    group_sound = lv_group_create();
    
    screen_sound = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_sound, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_sound);
    lv_label_set_text(title, "VOLUME");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn_back = lv_button_create(screen_sound);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_sound, btn_back);
    btn_sound_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *label_volume = lv_label_create(screen_sound);
    lv_label_set_text_fmt(label_volume, "%d%%", sound_volume);
    lv_obj_set_style_text_color(label_volume, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_volume, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_volume, LV_ALIGN_TOP_MID, 0, 55);

    slider_volume = lv_slider_create(screen_sound);
    lv_obj_set_size(slider_volume, 220, 25);
    lv_obj_align(slider_volume, LV_ALIGN_TOP_MID, 0, 90);
    lv_slider_set_range(slider_volume, 0, 100);
    lv_slider_set_value(slider_volume, sound_volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_volume, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_volume, lv_color_hex(0x003300), LV_PART_INDICATOR);
    lv_group_add_obj(group_sound, slider_volume);

    sw_mute = lv_switch_create(screen_sound);
    lv_obj_align(sw_mute, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(sw_mute, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_mute, lv_color_hex(0x003300), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw_mute, lv_color_hex(0x003300), LV_PART_KNOB);
    if (sound_mute) lv_obj_add_state(sw_mute, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw_mute, LV_STATE_CHECKED);
    lv_group_add_obj(group_sound, sw_mute);

    lv_obj_t *label_mute = lv_label_create(screen_sound);
    lv_label_set_text(label_mute, "MUTE");
    lv_obj_set_style_text_color(label_mute, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_mute, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align_to(label_mute, sw_mute, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_group_set_wrap(group_sound, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(slider_volume, lvgl_ui_volume_changed, LV_EVENT_VALUE_CHANGED, label_volume);
    lv_obj_add_event_cb(sw_mute, lvgl_ui_mute_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

void lvgl_ui_create_brightness_screen(void)
{
    group_brightness = lv_group_create();
    
    screen_brightness = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_brightness, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_brightness);
    lv_label_set_text(title, "BRIGHTNESS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn_back = lv_button_create(screen_brightness);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_brightness, btn_back);
    btn_bright_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *label_bright = lv_label_create(screen_brightness);
    lv_label_set_text_fmt(label_bright, "%d%%", lcd_brightness);
    lv_obj_set_style_text_color(label_bright, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_bright, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_bright, LV_ALIGN_TOP_MID, 0, 55);

    slider_bright = lv_slider_create(screen_brightness);
    lv_obj_set_size(slider_bright, 220, 25);
    lv_obj_align(slider_bright, LV_ALIGN_TOP_MID, 0, 90);
    lv_slider_set_range(slider_bright, 0, 100);
    lv_slider_set_value(slider_bright, lcd_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_bright, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_bright, lv_color_hex(0x003300), LV_PART_INDICATOR);
    lv_group_add_obj(group_brightness, slider_bright);

    lv_group_set_wrap(group_brightness, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(slider_bright, lvgl_ui_brightness_changed, LV_EVENT_VALUE_CHANGED, label_bright);
}

void lvgl_ui_create_wifi_screen(void)
{
    group_wifi = lv_group_create();
    
    screen_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_wifi, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_wifi);
    lv_label_set_text(title, "WIFI");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn_back = lv_button_create(screen_wifi);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_wifi, btn_back);
    btn_wifi_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    label_wifi_status = lv_label_create(screen_wifi);
    lv_label_set_text(label_wifi_status, wifi_connected ? "CONNECTED" : "DISCONNECTED");
    lv_obj_set_style_text_color(label_wifi_status, wifi_connected ? lv_color_hex(0x3fb950) : lv_color_hex(0xf85149), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_status, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_wifi_status, LV_ALIGN_TOP_MID, 0, 55);

    label_wifi_ip = lv_label_create(screen_wifi);
    lv_label_set_text_fmt(label_wifi_ip, "%s", wifi_ip[0] != '\0' ? wifi_ip : "NO IP");
    lv_obj_set_style_text_color(label_wifi_ip, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_ip, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_wifi_ip, LV_ALIGN_TOP_MID, 0, 95);

    btn_wifi_connect = lv_button_create(screen_wifi);
    lv_obj_set_size(btn_wifi_connect, 150, 40);
    lv_obj_set_style_bg_color(btn_wifi_connect, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_wifi_connect, 4, LV_PART_MAIN);
    lv_obj_align(btn_wifi_connect, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_group_add_obj(group_wifi, btn_wifi_connect);

    label_wifi_connect = lv_label_create(btn_wifi_connect);
    lv_label_set_text(label_wifi_connect, wifi_connected ? "DISCONNECT" : "CONNECT");
    lv_obj_set_style_text_color(label_wifi_connect, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_connect, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_group_set_wrap(group_wifi, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_show_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_wifi_connect, lvgl_ui_wifi_connect, LV_EVENT_CLICKED, NULL);
}

static void lvgl_ui_show_main(lv_event_t *e)
{
    lvgl_input_set_ppt_mode(false);
    if (ppt_controller_is_running()) {
        ppt_controller_stop();
    }
    lv_indev_set_group(s_indev, group_main);
    lv_screen_load(screen_main);
    if (btn_main_first) lv_group_focus_obj(btn_main_first);
}

static void lvgl_ui_show_settings(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_settings);
    lv_screen_load(screen_settings);
    if (btn_settings_first) lv_group_focus_obj(btn_settings_first);
}

static void lvgl_ui_show_smart(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_smart);
    lv_screen_load(screen_smart);
    if (btn_smart_first) lv_group_focus_obj(btn_smart_first);
}

static void lvgl_ui_show_chat(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_chat);
    lv_screen_load(screen_chat);
    if (btn_chat_first) lv_group_focus_obj(btn_chat_first);
}

static void lvgl_ui_show_sound(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_sound);
    lv_screen_load(screen_sound);
    if (btn_sound_first) lv_group_focus_obj(btn_sound_first);
}

static void lvgl_ui_show_brightness(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_brightness);
    lv_screen_load(screen_brightness);
    if (btn_bright_first) lv_group_focus_obj(btn_bright_first);
}

static void lvgl_ui_show_wifi(lv_event_t *e)
{
    lv_indev_set_group(s_indev, group_wifi);
    lv_screen_load(screen_wifi);
    if (btn_wifi_first) lv_group_focus_obj(btn_wifi_first);
}

static void lvgl_ui_volume_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    sound_volume = lv_slider_get_value(slider);
    audio_set_volume(sound_volume);
    lv_label_set_text_fmt(label, "Volume: %d%%", sound_volume);
}

static void lvgl_ui_mute_changed(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    sound_mute = lv_obj_has_state(sw, LV_STATE_CHECKED);
    audio_set_mute(sound_mute);
}

static void lvgl_ui_brightness_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    lcd_brightness = lv_slider_get_value(slider);
    lv_label_set_text_fmt(label, "Brightness: %d%%", lcd_brightness);
    lvgl_display_set_brightness(lcd_brightness);
}

static void lvgl_ui_start_voice_chat(lv_event_t *e)
{
    voice_record_start();
}

static void lvgl_ui_wifi_connect(lv_event_t *e)
{
    if (!wifi_connected) {
        extern void wifi_connect(void);
        wifi_connect();
    } else {
        esp_wifi_disconnect();
    }
}

void lvgl_ui_update_wifi(void)
{
    if (label_wifi_status) {
        lv_label_set_text(label_wifi_status, wifi_connected ? "Connected" : "Disconnected");
        lv_obj_set_style_text_color(label_wifi_status, wifi_connected ? lv_color_hex(0x3fb950) : lv_color_hex(0xf85149), LV_PART_MAIN);
    }
    if (label_wifi_ip) {
        lv_label_set_text_fmt(label_wifi_ip, "IP: %s", wifi_ip[0] != '\0' ? wifi_ip : "Unknown");
    }
    if (btn_wifi_connect) {
        lv_obj_set_style_bg_color(btn_wifi_connect, lv_color_hex(0x003300), LV_PART_MAIN);
    }
    if (label_wifi_connect) {
        lv_label_set_text(label_wifi_connect, wifi_connected ? "Disconnect" : "Connect");
    }
}

static void lvgl_ui_ppt_control(lv_event_t *e)
{
    ppt_controller_start();
    lvgl_input_set_ppt_mode(true);
    lvgl_ui_ppt_update_status("Ready");
    lv_screen_load(screen_ppt_control);
}

static void lvgl_ui_ppt_exit_cb(void)
{
    lv_indev_set_group(s_indev, group_smart);
    lv_screen_load(screen_smart);
}

static void lvgl_ui_aircon_control(lv_event_t *e)
{
    lv_textarea_set_text(chat_textarea, "");
    lvgl_ui_append_chat_message("Air Con Control Ready");
    lv_screen_load(screen_chat);
}

static void update_ppt_ip_label(void)
{
    if (!label_ppt_ip) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", 
             ppt_ip_octets[0], ppt_ip_octets[1], 
             ppt_ip_octets[2], ppt_ip_octets[3]);
    lv_label_set_text(label_ppt_ip, buf);
}

static void update_ppt_port_label(void)
{
    if (!label_ppt_port) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", ppt_port_val);
    lv_label_set_text(label_ppt_port, buf);
}

static lv_obj_t *btn_ip1, *btn_ip2, *btn_ip3, *btn_ip4, *btn_port_edit;
static bool ppt_editing = false;

static void set_ppt_field_style(int field, bool editing)
{
    lv_obj_t *btns[5] = {btn_ip1, btn_ip2, btn_ip3, btn_ip4, btn_port_edit};
    if (field < 0 || field >= 5 || !btns[field]) return;
    
    if (editing) {
        lv_obj_set_style_bg_color(btns[field], lv_color_hex(0x00aa00), LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(btns[field], lv_color_hex(0x003300), LV_PART_MAIN);
    }
}

static void lvgl_ui_create_ppt_settings_screen(void)
{
    group_ppt_settings = lv_group_create();
    
    screen_ppt_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_ppt_settings, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_ppt_settings);
    lv_label_set_text(title, "PPT SERVER");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *btn_back = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_back, 50, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x002200), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_group_add_obj(group_ppt_settings, btn_back);
    btn_ppt_first = btn_back;

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *label_ip_title = lv_label_create(screen_ppt_settings);
    lv_label_set_text(label_ip_title, "IP Address");
    lv_obj_set_style_text_color(label_ip_title, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ip_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_ip_title, LV_ALIGN_TOP_MID, 0, 55);

    label_ppt_ip = lv_label_create(screen_ppt_settings);
    lv_obj_set_style_text_color(label_ppt_ip, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ppt_ip, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_ppt_ip, LV_ALIGN_TOP_MID, 0, 78);

    lv_obj_t *label_port_title = lv_label_create(screen_ppt_settings);
    lv_label_set_text(label_port_title, "Port");
    lv_obj_set_style_text_color(label_port_title, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_port_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_port_title, LV_ALIGN_TOP_MID, 0, 120);

    label_ppt_port = lv_label_create(screen_ppt_settings);
    lv_obj_set_style_text_color(label_ppt_port, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ppt_port, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(label_ppt_port, LV_ALIGN_TOP_MID, 0, 143);

    btn_ip1 = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_ip1, 40, 32);
    lv_obj_set_style_bg_color(btn_ip1, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ip1, lv_color_hex(0x196c2e), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ip1, 4, LV_PART_MAIN);
    lv_obj_align(btn_ip1, LV_ALIGN_TOP_MID, -75, 180);
    lv_group_add_obj(group_ppt_settings, btn_ip1);
    lv_obj_t *lb1 = lv_label_create(btn_ip1);
    lv_label_set_text(lb1, "IP1");
    lv_obj_set_style_text_color(lb1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb1, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(lb1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_ip1, lvgl_ui_ppt_field_event, LV_EVENT_ALL, (void *)0);

    btn_ip2 = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_ip2, 40, 32);
    lv_obj_set_style_bg_color(btn_ip2, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ip2, lv_color_hex(0x196c2e), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ip2, 4, LV_PART_MAIN);
    lv_obj_align(btn_ip2, LV_ALIGN_TOP_MID, -25, 180);
    lv_group_add_obj(group_ppt_settings, btn_ip2);
    lv_obj_t *lb2 = lv_label_create(btn_ip2);
    lv_label_set_text(lb2, "IP2");
    lv_obj_set_style_text_color(lb2, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb2, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(lb2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_ip2, lvgl_ui_ppt_field_event, LV_EVENT_ALL, (void *)1);

    btn_ip3 = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_ip3, 40, 32);
    lv_obj_set_style_bg_color(btn_ip3, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ip3, lv_color_hex(0x196c2e), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ip3, 4, LV_PART_MAIN);
    lv_obj_align(btn_ip3, LV_ALIGN_TOP_MID, 25, 180);
    lv_group_add_obj(group_ppt_settings, btn_ip3);
    lv_obj_t *lb3 = lv_label_create(btn_ip3);
    lv_label_set_text(lb3, "IP3");
    lv_obj_set_style_text_color(lb3, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb3, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(lb3, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_ip3, lvgl_ui_ppt_field_event, LV_EVENT_ALL, (void *)2);

    btn_ip4 = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_ip4, 40, 32);
    lv_obj_set_style_bg_color(btn_ip4, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ip4, lv_color_hex(0x196c2e), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_ip4, 4, LV_PART_MAIN);
    lv_obj_align(btn_ip4, LV_ALIGN_TOP_MID, 75, 180);
    lv_group_add_obj(group_ppt_settings, btn_ip4);
    lv_obj_t *lb4 = lv_label_create(btn_ip4);
    lv_label_set_text(lb4, "IP4");
    lv_obj_set_style_text_color(lb4, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb4, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(lb4, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_ip4, lvgl_ui_ppt_field_event, LV_EVENT_ALL, (void *)3);

    btn_port_edit = lv_button_create(screen_ppt_settings);
    lv_obj_set_size(btn_port_edit, 100, 32);
    lv_obj_set_style_bg_color(btn_port_edit, lv_color_hex(0x003300), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_port_edit, lv_color_hex(0x196c2e), LV_PART_SELECTED);
    lv_obj_set_style_radius(btn_port_edit, 4, LV_PART_MAIN);
    lv_obj_align(btn_port_edit, LV_ALIGN_TOP_MID, 0, 220);
    lv_group_add_obj(group_ppt_settings, btn_port_edit);
    lv_obj_t *lbp = lv_label_create(btn_port_edit);
    lv_label_set_text(lbp, "PORT");
    lv_obj_set_style_text_color(lbp, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbp, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(lbp, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_port_edit, lvgl_ui_ppt_field_event, LV_EVENT_ALL, (void *)4);

    lv_obj_t *hint = lv_label_create(screen_ppt_settings);
    lv_label_set_text(hint, "Select field, press to edit, rotate to adjust");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 258);

    lv_group_set_wrap(group_ppt_settings, true);

    lv_obj_add_event_cb(btn_back, lvgl_ui_ppt_settings_back, LV_EVENT_CLICKED, NULL);
}

static void lvgl_ui_create_ppt_control_screen(void)
{
    screen_ppt_control = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_ppt_control, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_ppt_control);
    lv_label_set_text(title, "PPT REMOTE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    label_ppt_status = lv_label_create(screen_ppt_control);
    lv_label_set_text(label_ppt_status, "Ready");
    lv_obj_set_style_text_color(label_ppt_status, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ppt_status, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_ppt_status, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *icon_up = lv_label_create(screen_ppt_control);
    lv_label_set_text(icon_up, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(icon_up, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon_up, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(icon_up, LV_ALIGN_CENTER, 0, -45);

    lv_obj_t *label_next = lv_label_create(screen_ppt_control);
    lv_label_set_text(label_next, "NEXT");
    lv_obj_set_style_text_color(label_next, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_next, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label_prev = lv_label_create(screen_ppt_control);
    lv_label_set_text(label_prev, "PREV");
    lv_obj_set_style_text_color(label_prev, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_prev, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *icon_down = lv_label_create(screen_ppt_control);
    lv_label_set_text(icon_down, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(icon_down, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon_down, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(icon_down, LV_ALIGN_CENTER, 0, 35);

    lv_obj_t *hint1 = lv_label_create(screen_ppt_control);
    lv_label_set_text(hint1, "Rotate: Next / Prev");
    lv_obj_set_style_text_color(hint1, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint1, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(hint1, LV_ALIGN_BOTTOM_MID, 0, 50);

    lv_obj_t *hint2 = lv_label_create(screen_ppt_control);
    lv_label_set_text(hint2, "Click: Vol+  Double: Vol-");
    lv_obj_set_style_text_color(hint2, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint2, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(hint2, LV_ALIGN_BOTTOM_MID, 0, 30);

    lv_obj_t *hint3 = lv_label_create(screen_ppt_control);
    lv_label_set_text(hint3, "Long Press: Exit");
    lv_obj_set_style_text_color(hint3, lv_color_hex(0xf85149), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint3, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(hint3, LV_ALIGN_BOTTOM_MID, 0, 10);
}

void lvgl_ui_ppt_update_status(const char *status)
{
    if (label_ppt_status && status) {
        lv_label_set_text(label_ppt_status, status);
    }
}

static void lvgl_ui_ppt_field_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int field = (int)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_CLICKED) {
        ppt_editing = !ppt_editing;
        if (ppt_editing) {
            ppt_edit_field = field;
        }
        set_ppt_field_style(ppt_edit_field, ppt_editing);
    } else if (code == LV_EVENT_KEY && ppt_editing && field == ppt_edit_field) {
        uint32_t key = lv_indev_get_key(lv_indev_active());
        if (key == LV_KEY_UP || key == LV_KEY_RIGHT) {
            if (field < 4) {
                ppt_ip_octets[field] = (ppt_ip_octets[field] + 1) % 256;
                update_ppt_ip_label();
            } else {
                ppt_port_val++;
                if (ppt_port_val > 65535) ppt_port_val = 65535;
                update_ppt_port_label();
            }
        } else if (key == LV_KEY_DOWN || key == LV_KEY_LEFT) {
            if (field < 4) {
                if (ppt_ip_octets[field] == 0) ppt_ip_octets[field] = 255;
                else ppt_ip_octets[field]--;
                update_ppt_ip_label();
            } else {
                ppt_port_val--;
                if (ppt_port_val < 1) ppt_port_val = 1;
                update_ppt_port_label();
            }
        }
    }
}

static void lvgl_ui_show_ppt_settings(lv_event_t *e)
{
    char ip[16] = {0};
    int port = 0;
    ppt_controller_get_server(ip, &port);
    
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        ppt_ip_octets[0] = a;
        ppt_ip_octets[1] = b;
        ppt_ip_octets[2] = c;
        ppt_ip_octets[3] = d;
    }
    ppt_port_val = port;
    ppt_edit_field = -1;
    ppt_editing = false;
    
    update_ppt_ip_label();
    update_ppt_port_label();
    
    lv_indev_set_group(s_indev, group_ppt_settings);
    lv_screen_load(screen_ppt_settings);
    if (btn_ppt_first) lv_group_focus_obj(btn_ppt_first);
}

static void lvgl_ui_ppt_settings_back(lv_event_t *e)
{
    char ip[16];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
             ppt_ip_octets[0], ppt_ip_octets[1],
             ppt_ip_octets[2], ppt_ip_octets[3]);
    ppt_controller_set_server(ip, ppt_port_val);
    
    lv_indev_set_group(s_indev, group_settings);
    lv_screen_load(screen_settings);
    if (btn_settings_first) lv_group_focus_obj(btn_settings_first);
}

void lvgl_ui_append_chat_message(const char *msg)
{
    if (chat_textarea) {
        lv_textarea_add_text(chat_textarea, msg);
        lv_textarea_add_text(chat_textarea, "\n");
    }
}

static void voice_chat_response_cb(const char *response)
{
    if (response && strlen(response) > 0) {
        lvgl_ui_append_chat_message(response);
    }
}

void lvgl_ui_init(void)
{
    s_indev = lvgl_input_get_indev();
    
    lvgl_ui_create_main_screen();
    lvgl_ui_create_settings_screen();
    lvgl_ui_create_smart_screen();
    lvgl_ui_create_chat_screen();
    lvgl_ui_create_sound_screen();
    lvgl_ui_create_brightness_screen();
    lvgl_ui_create_wifi_screen();
    lvgl_ui_create_ppt_settings_screen();
    lvgl_ui_create_ppt_control_screen();
    
    voice_set_response_callback(voice_chat_response_cb);
    
    lvgl_input_set_ppt_exit_callback(lvgl_ui_ppt_exit_cb);
    
    lv_indev_set_group(s_indev, group_main);
    lv_screen_load(screen_main);
    if (btn_main_first) lv_group_focus_obj(btn_main_first);
}