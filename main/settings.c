#include "settings.h"
#include "home.h"
#include "wifi.h"
#include "night.h"
#include "block.h"
#include "clock.h"
#include "price.h"
#include "mempool.h"
#include "stdio.h"
#include "string.h"
#include "custom_fonts.h"
#include "bap.h"
#include "waveshare_rgb_lcd_port.h"
#include <stdlib.h>
#include <time.h>
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "settings_screen";

static lv_obj_t *settings_screen = NULL;
static lv_obj_t *performance_low_btn = NULL;
static lv_obj_t *performance_medium_btn = NULL;
static lv_obj_t *performance_high_btn = NULL;
static lv_obj_t *auto_fan_checkbox = NULL;
static lv_obj_t *fan_slider = NULL;
static lv_obj_t *fan_value_label = NULL;
static lv_obj_t *fan_save_btn = NULL;
static lv_obj_t *brightness_slider = NULL;
static lv_obj_t *brightness_value_label = NULL;
static lv_obj_t *timezone_dropdown = NULL;
static lv_obj_t *sys_overlay = NULL;
static int diag_counter = 0;

static settings_info_t current_settings = {
    .performance_mode = PERFORMANCE_MEDIUM,
    .auto_fan_control = true,
    .fan_speed_percent = 50,
    .brightness_percent = 100};

static int current_timezone_index = 0;
static bool timezone_applied = false;

static const char *timezone_options =
    "UTC\n"
    "US/Pacific\n"
    "US/Mountain\n"
    "US/Central\n"
    "US/Eastern\n"
    "Europe/London\n"
    "Europe/Berlin\n"
    "Asia/Tokyo\n"
    "Australia/Sydney";

static const char *timezone_values[] = {
    "UTC0",
    "PST8PDT,M3.2.0/2,M11.1.0/2",
    "MST7MDT,M3.2.0/2,M11.1.0/2",
    "CST6CDT,M3.2.0/2,M11.1.0/2",
    "EST5EDT,M3.2.0/2,M11.1.0/2",
    "GMT0BST,M3.5.0/1,M10.5.0/2",
    "CET-1CEST,M3.5.0/2,M10.5.0/3",
    "JST-9",
    "AEST-10AEDT,M10.1.0/2,M4.1.0/3",
};

#define SETTINGS_NVS_NAMESPACE "settings"
#define SETTINGS_NVS_TZ_INDEX_KEY "tz_index"

static lv_obj_t *create_settings_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 170, 48);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 56, 46);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

static void update_performance_buttons(void)
{
    if (!performance_low_btn || !performance_medium_btn || !performance_high_btn)
        return;

    lv_obj_set_style_bg_color(performance_low_btn, COLOR_CARD_BG, 0);
    lv_obj_t *low_label = lv_obj_get_child(performance_low_btn, 0);
    if (low_label)
        lv_obj_set_style_text_color(low_label, COLOR_ACCENT, 0);

    lv_obj_set_style_bg_color(performance_medium_btn, COLOR_CARD_BG, 0);
    lv_obj_t *medium_label = lv_obj_get_child(performance_medium_btn, 0);
    if (medium_label)
        lv_obj_set_style_text_color(medium_label, COLOR_ACCENT, 0);

    lv_obj_set_style_bg_color(performance_high_btn, COLOR_CARD_BG, 0);
    lv_obj_t *high_label = lv_obj_get_child(performance_high_btn, 0);
    if (high_label)
        lv_obj_set_style_text_color(high_label, COLOR_ACCENT, 0);

    lv_obj_t *active_btn = NULL;
    lv_obj_t *active_label = NULL;

    switch (current_settings.performance_mode)
    {
    case PERFORMANCE_LOW:
        active_btn = performance_low_btn;
        active_label = low_label;
        break;
    case PERFORMANCE_MEDIUM:
        active_btn = performance_medium_btn;
        active_label = medium_label;
        break;
    case PERFORMANCE_HIGH:
        active_btn = performance_high_btn;
        active_label = high_label;
        break;
    }

    if (active_btn && active_label)
    {
        lv_obj_set_style_bg_color(active_btn, COLOR_ACCENT, 0);
        lv_obj_set_style_border_opa(active_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(active_label, COLOR_TEXT_ON_ACCENT, 0);
    }
}

static void apply_timezone_by_index(int index)
{
    size_t tz_count = sizeof(timezone_values) / sizeof(timezone_values[0]);
    if (index < 0 || (size_t)index >= tz_count)
    {
        return;
    }

    setenv("TZ", timezone_values[index], 1);
    tzset();
    timezone_applied = true;
}

static void settings_load_timezone(void)
{
    static bool nvs_ready = false;
    if (!nvs_ready)
    {
        esp_err_t init_err = nvs_flash_init();
        if (init_err == ESP_ERR_NVS_NO_FREE_PAGES || init_err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            init_err = nvs_flash_init();
        }
        if (init_err != ESP_OK)
        {
            return;
        }
        nvs_ready = true;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return;
    }

    int32_t saved_index = 0;
    err = nvs_get_i32(handle, SETTINGS_NVS_TZ_INDEX_KEY, &saved_index);
    nvs_close(handle);
    if (err == ESP_OK)
    {
        current_timezone_index = (int)saved_index;
    }
}

static void settings_save_timezone(int index)
{
    static bool nvs_ready = false;
    if (!nvs_ready)
    {
        esp_err_t init_err = nvs_flash_init();
        if (init_err == ESP_ERR_NVS_NO_FREE_PAGES || init_err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            init_err = nvs_flash_init();
        }
        if (init_err != ESP_OK)
        {
            return;
        }
        nvs_ready = true;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return;
    }

    nvs_set_i32(handle, SETTINGS_NVS_TZ_INDEX_KEY, index);
    nvs_commit(handle);
    nvs_close(handle);
}

static void update_fan_controls(void)
{
    if (!fan_slider || !fan_value_label)
        return;

    if (current_settings.auto_fan_control)
    {
        lv_obj_add_flag(fan_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(fan_value_label, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(fan_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(fan_value_label, LV_OBJ_FLAG_HIDDEN);

        lv_slider_set_value(fan_slider, current_settings.fan_speed_percent, LV_ANIM_OFF);
        char fan_text[16];
        snprintf(fan_text, sizeof(fan_text), "%d%%", current_settings.fan_speed_percent);
        lv_label_set_text(fan_value_label, fan_text);
    }
}

static void decode_sys_info(char *output, size_t output_size)
{
    static const uint8_t encoded_data[] = {
        38, 39, 52, 39, 46, 45, 50, 39, 38, 98,
        32, 59, 98, 21, 35, 44, 54, 1, 46, 55, 39
    };
    const uint8_t key = 0x42;
    size_t len = sizeof(encoded_data);

    for (size_t i = 0; i < len && i < output_size - 1; i++) {
        output[i] = encoded_data[i] ^ key;
    }
    output[len < output_size ? len : output_size - 1] = '\0';
}

static void cleanup_system_overlay(lv_event_t *e)
{
    if (sys_overlay) {
        lv_obj_del(sys_overlay);
        sys_overlay = NULL;
    }
    diag_counter = 0;
}

static void create_system_overlay(void)
{
    if (sys_overlay) {
        return;
    }

    sys_overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(sys_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(sys_overlay, 0, 0);
    lv_obj_set_style_bg_color(sys_overlay, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(sys_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(sys_overlay, 0, 0);
    lv_obj_clear_flag(sys_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(sys_overlay, cleanup_system_overlay, LV_EVENT_CLICKED, NULL);

    lv_obj_t *dialog = lv_obj_create(sys_overlay);
    lv_obj_set_size(dialog, 400, 200);
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_border_color(dialog, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dialog, 14, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    char display_buffer[64];
    decode_sys_info(display_buffer, sizeof(display_buffer));

    lv_obj_t *label = lv_label_create(dialog);
    lv_label_set_text(label, display_buffer);
    lv_obj_set_style_text_color(label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, 0);
    lv_obj_center(label);
}

static void settings_diagnostics_handler(lv_event_t *e)
{
    diag_counter++;

    if (diag_counter >= 3) {
        create_system_overlay();
        diag_counter = 0;
    }
}

void settings_screen_create(void)
{
    if (settings_screen != NULL)
    {
        return;
    }

    settings_load_timezone();

    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(settings_screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *main_cont = lv_obj_create(settings_screen);
    lv_obj_set_size(main_cont, SCREEN_WIDTH - 60, SCREEN_HEIGHT - 100);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(main_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(main_cont, 1, 0);
    lv_obj_set_style_border_color(main_cont, COLOR_BORDER, 0);
    lv_obj_set_style_border_opa(main_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(main_cont, 14, 0);
    lv_obj_set_style_pad_all(main_cont, 16, 0);
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(main_cont, LV_DIR_VER);
    lv_obj_set_style_pad_bottom(main_cont, 80, 0);

    lv_obj_t *title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "SETTINGS");
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *perf_section = lv_obj_create(main_cont);
    lv_obj_set_size(perf_section, 680, 110);
    lv_obj_align(perf_section, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_bg_opa(perf_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(perf_section, 0, 0);
    lv_obj_set_style_pad_all(perf_section, 10, 0);
    lv_obj_clear_flag(perf_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *perf_title = lv_label_create(perf_section);
    lv_label_set_text(perf_title, "Performance Mode:");
    lv_obj_set_style_text_color(perf_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(perf_title, &lv_font_montserrat_18, 0);
    lv_obj_align(perf_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *perf_btn_cont = lv_obj_create(perf_section);
    lv_obj_set_size(perf_btn_cont, 560, 56);
    lv_obj_align(perf_btn_cont, LV_ALIGN_TOP_LEFT, 0, 32);
    lv_obj_set_style_bg_opa(perf_btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(perf_btn_cont, 0, 0);
    lv_obj_set_style_pad_all(perf_btn_cont, 0, 0);
    lv_obj_set_flex_flow(perf_btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(perf_btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    performance_low_btn = create_settings_button(perf_btn_cont, "LOW", settings_performance_low_clicked,
                                                 current_settings.performance_mode == PERFORMANCE_LOW);
    performance_medium_btn = create_settings_button(perf_btn_cont, "MEDIUM", settings_performance_medium_clicked,
                                                    current_settings.performance_mode == PERFORMANCE_MEDIUM);
    performance_high_btn = create_settings_button(perf_btn_cont, "HIGH", settings_performance_high_clicked,
                                                  current_settings.performance_mode == PERFORMANCE_HIGH);

    lv_obj_t *fan_section = lv_obj_create(main_cont);
    lv_obj_set_size(fan_section, 680, 200);
    lv_obj_align(fan_section, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_bg_opa(fan_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fan_section, 0, 0);
    lv_obj_set_style_pad_all(fan_section, 10, 0);
    lv_obj_clear_flag(fan_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *fan_title = lv_label_create(fan_section);
    lv_label_set_text(fan_title, "Fan Control:");
    lv_obj_set_style_text_color(fan_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(fan_title, &lv_font_montserrat_18, 0);
    lv_obj_align(fan_title, LV_ALIGN_TOP_LEFT, 0, 0);

    auto_fan_checkbox = lv_checkbox_create(fan_section);
    lv_checkbox_set_text(auto_fan_checkbox, "Automatic Fan Control");
    lv_obj_set_style_text_color(auto_fan_checkbox, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(auto_fan_checkbox, &lv_font_montserrat_16, 0);
    lv_obj_align(auto_fan_checkbox, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_add_event_cb(auto_fan_checkbox, settings_auto_fan_toggled, LV_EVENT_VALUE_CHANGED, NULL);

    if (current_settings.auto_fan_control)
    {
        lv_obj_add_state(auto_fan_checkbox, LV_STATE_CHECKED);
    }

    fan_slider = lv_slider_create(fan_section);
    lv_obj_set_size(fan_slider, 420, 20);
    lv_obj_align(fan_slider, LV_ALIGN_TOP_LEFT, 0, 70);
    lv_slider_set_range(fan_slider, 0, 100);
    lv_slider_set_value(fan_slider, current_settings.fan_speed_percent, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(fan_slider, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(fan_slider, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(fan_slider, COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(fan_slider, settings_fan_slider_changed, LV_EVENT_VALUE_CHANGED, NULL);

    fan_value_label = lv_label_create(fan_section);
    char fan_text[16];
    snprintf(fan_text, sizeof(fan_text), "%d%%", current_settings.fan_speed_percent);
    lv_label_set_text(fan_value_label, fan_text);
    lv_obj_set_style_text_color(fan_value_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(fan_value_label, &lv_font_montserrat_16, 0);
    lv_obj_align(fan_value_label, LV_ALIGN_TOP_LEFT, 450, 65);

    fan_save_btn = create_settings_button(fan_section, "SAVE FAN SETTINGS", settings_fan_save_clicked, false);
    lv_obj_set_size(fan_save_btn, 220, 36);
    lv_obj_align(fan_save_btn, LV_ALIGN_TOP_LEFT, 0, 115);

    update_fan_controls();

    lv_obj_t *brightness_section = lv_obj_create(main_cont);
    lv_obj_set_size(brightness_section, 680, 70);
    lv_obj_align(brightness_section, LV_ALIGN_TOP_MID, 0, 350);
    lv_obj_set_style_bg_opa(brightness_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(brightness_section, 0, 0);
    lv_obj_set_style_pad_all(brightness_section, 10, 0);
    lv_obj_clear_flag(brightness_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brightness_title = lv_label_create(brightness_section);
    lv_label_set_text(brightness_title, "Screen Brightness:");
    lv_obj_set_style_text_color(brightness_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(brightness_title, &lv_font_montserrat_18, 0);
    lv_obj_align(brightness_title, LV_ALIGN_TOP_LEFT, 0, 0);

    brightness_slider = lv_slider_create(brightness_section);
    lv_obj_set_size(brightness_slider, 550, 20);
    lv_obj_align(brightness_slider, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_slider_set_range(brightness_slider, 5, 100);
    lv_slider_set_value(brightness_slider, current_settings.brightness_percent, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightness_slider, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightness_slider, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider, COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(brightness_slider, settings_brightness_slider_changed, LV_EVENT_VALUE_CHANGED, NULL);

    brightness_value_label = lv_label_create(brightness_section);
    char brightness_text[16];
    snprintf(brightness_text, sizeof(brightness_text), "%d%%", current_settings.brightness_percent);
    lv_label_set_text(brightness_value_label, brightness_text);
    lv_obj_set_style_text_color(brightness_value_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(brightness_value_label, &lv_font_montserrat_22, 0);
    lv_obj_align(brightness_value_label, LV_ALIGN_TOP_LEFT, 600, 26);

    lv_obj_t *timezone_section = lv_obj_create(main_cont);
    lv_obj_set_size(timezone_section, 680, 50);
    lv_obj_align(timezone_section, LV_ALIGN_TOP_MID, 0, 430);
    lv_obj_set_style_bg_opa(timezone_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(timezone_section, 0, 0);
    lv_obj_set_style_pad_all(timezone_section, 10, 0);
    lv_obj_clear_flag(timezone_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *timezone_title = lv_label_create(timezone_section);
    lv_label_set_text(timezone_title, "Time Zone:");
    lv_obj_set_style_text_color(timezone_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(timezone_title, &lv_font_montserrat_18, 0);
    lv_obj_align(timezone_title, LV_ALIGN_TOP_LEFT, 0, 0);

    timezone_dropdown = lv_dropdown_create(timezone_section);
    lv_obj_set_size(timezone_dropdown, 300, 34);
    lv_obj_align(timezone_dropdown, LV_ALIGN_TOP_LEFT, 140, -4);
    lv_dropdown_set_options(timezone_dropdown, timezone_options);
    lv_dropdown_set_selected(timezone_dropdown, current_timezone_index);
    lv_obj_set_style_bg_color(timezone_dropdown, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(timezone_dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(timezone_dropdown, 1, 0);
    lv_obj_set_style_border_color(timezone_dropdown, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(timezone_dropdown, LV_OPA_50, 0);
    lv_obj_set_style_radius(timezone_dropdown, 8, 0);
    lv_obj_set_style_text_color(timezone_dropdown, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(timezone_dropdown, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(timezone_dropdown, settings_timezone_changed, LV_EVENT_VALUE_CHANGED, NULL);

    if (!timezone_applied)
    {
        apply_timezone_by_index(current_timezone_index);
    }

    lv_obj_t *bottom_nav = lv_obj_create(settings_screen);
    lv_obj_set_size(bottom_nav, SCREEN_WIDTH, 64);
    lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_nav, COLOR_NAV_BG, 0);
    lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_nav, 0, 0);
    lv_obj_set_style_radius(bottom_nav, 0, 0);
    lv_obj_set_style_pad_all(bottom_nav, 8, 0);
    lv_obj_clear_flag(bottom_nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(bottom_nav, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, settings_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_STOP, settings_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, settings_mempool_clicked, false);
    create_bottom_nav_btn(bottom_nav, FA_CLOCK, settings_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", settings_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, settings_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, settings_diagnostics_handler, true);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, settings_night_clicked, false);
}

void settings_screen_destroy(void)
{
    // Clean up Easter egg overlay if showing
    if (sys_overlay) {
        lv_obj_del(sys_overlay);
        sys_overlay = NULL;
    }
    diag_counter = 0;

    if (settings_screen)
    {
        lv_obj_del(settings_screen);
        settings_screen = NULL;
        performance_low_btn = NULL;
        performance_medium_btn = NULL;
        performance_high_btn = NULL;
        auto_fan_checkbox = NULL;
        fan_slider = NULL;
        fan_value_label = NULL;
        fan_save_btn = NULL;
        brightness_slider = NULL;
        brightness_value_label = NULL;
        timezone_dropdown = NULL;
    }
}

lv_obj_t *settings_get_screen(void)
{
    return settings_screen;
}

void settings_update_info(const settings_info_t *info)
{
    if (info)
    {
        current_settings = *info;
        update_performance_buttons();
        update_fan_controls();

        if (auto_fan_checkbox)
        {
            if (current_settings.auto_fan_control)
            {
                lv_obj_add_state(auto_fan_checkbox, LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_clear_state(auto_fan_checkbox, LV_STATE_CHECKED);
            }
        }

        if (brightness_slider)
        {
            lv_slider_set_value(brightness_slider, current_settings.brightness_percent, LV_ANIM_OFF);
        }
        if (brightness_value_label)
        {
            char brightness_text[16];
            snprintf(brightness_text, sizeof(brightness_text), "%d%%", current_settings.brightness_percent);
            lv_label_set_text(brightness_value_label, brightness_text);
        }
    }
}

void settings_performance_low_clicked(lv_event_t *e)
{
    current_settings.performance_mode = PERFORMANCE_LOW;
    update_performance_buttons();
    printf("Performance mode set to LOW\n");

    BAP_send_frequency_setting(575.0f);
    BAP_send_asic_voltage(1160.0f);
}

void settings_performance_medium_clicked(lv_event_t *e)
{
    current_settings.performance_mode = PERFORMANCE_MEDIUM;
    update_performance_buttons();
    printf("Performance mode set to MEDIUM\n");

    BAP_send_frequency_setting(600.0f);
    BAP_send_asic_voltage(1200.0f);
}

void settings_performance_high_clicked(lv_event_t *e)
{
    current_settings.performance_mode = PERFORMANCE_HIGH;
    update_performance_buttons();
    printf("Performance mode set to HIGH\n");

    BAP_send_frequency_setting(655.0f);
    BAP_send_asic_voltage(1200.0f);
}

void settings_auto_fan_toggled(lv_event_t *e)
{
    lv_obj_t *checkbox = lv_event_get_target(e);
    current_settings.auto_fan_control = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    update_fan_controls();
    printf("Auto fan control: %s\n", current_settings.auto_fan_control ? "ON" : "OFF");
}

void settings_fan_slider_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    current_settings.fan_speed_percent = lv_slider_get_value(slider);

    if (fan_value_label)
    {
        char fan_text[16];
        snprintf(fan_text, sizeof(fan_text), "%d%%", current_settings.fan_speed_percent);
        lv_label_set_text(fan_value_label, fan_text);
    }

    printf("Fan speed set to: %d%%\n", current_settings.fan_speed_percent);
}

void settings_fan_save_clicked(lv_event_t *e)
{
    printf("Saving fan settings - Auto: %s, Speed: %d%%\n",
           current_settings.auto_fan_control ? "ON" : "OFF",
           current_settings.fan_speed_percent);

    if (current_settings.auto_fan_control)
    {
        BAP_send_automatic_fan_control(true);
        printf("Sending auto fan control command\n");
    }
    else
    {
        BAP_send_fan_speed(current_settings.fan_speed_percent);
        printf("Sending manual fan speed: %d%%\n", current_settings.fan_speed_percent);
    }
}

void settings_home_clicked(lv_event_t *e)
{
    home_screen_create();
    lv_scr_load(home_get_screen());
    settings_screen_destroy();
}

void settings_wifi_clicked(lv_event_t *e)
{
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    settings_screen_destroy();
}

void settings_clock_clicked(lv_event_t *e)
{
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    settings_screen_destroy();
}

void settings_price_clicked(lv_event_t *e)
{
    price_screen_create();
    lv_scr_load(price_get_screen());
    settings_screen_destroy();
}

void settings_block_clicked(lv_event_t *e)
{
    block_screen_create();
    lv_scr_load(block_get_screen());
    settings_screen_destroy();
}

void settings_mempool_clicked(lv_event_t *e)
{
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    settings_screen_destroy();
}

void settings_brightness_slider_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    current_settings.brightness_percent = lv_slider_get_value(slider);

    if (brightness_value_label)
    {
        char brightness_text[16];
        snprintf(brightness_text, sizeof(brightness_text), "%d%%", current_settings.brightness_percent);
        lv_label_set_text(brightness_value_label, brightness_text);
    }

    // Apply brightness change immediately
    lcd_backlight_set_brightness(current_settings.brightness_percent);

    printf("Screen brightness set to: %d%%\n", current_settings.brightness_percent);
}

void settings_timezone_changed(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    current_timezone_index = (int)lv_dropdown_get_selected(dropdown);
    apply_timezone_by_index(current_timezone_index);
    settings_save_timezone(current_timezone_index);
}

void settings_night_clicked(lv_event_t *e)
{
    // Navigate to night mode screen
    night_screen_create();
    lv_scr_load(night_get_screen());
    settings_screen_destroy();
}
