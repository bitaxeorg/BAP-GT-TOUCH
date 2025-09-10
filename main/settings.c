#include "settings.h"
#include "home.h"
#include "wifi.h"
#include "night.h"
#include "stdio.h"
#include "string.h"
#include "custom_fonts.h"
#include "bap.h"
#include "waveshare_rgb_lcd_port.h"

static const char* TAG = "settings_screen";

static lv_obj_t * settings_screen = NULL;
static lv_obj_t * performance_low_btn = NULL;
static lv_obj_t * performance_medium_btn = NULL;
static lv_obj_t * performance_high_btn = NULL;
static lv_obj_t * auto_fan_checkbox = NULL;
static lv_obj_t * fan_slider = NULL;
static lv_obj_t * fan_value_label = NULL;
static lv_obj_t * fan_save_btn = NULL;
static lv_obj_t * brightness_slider = NULL;
static lv_obj_t * brightness_value_label = NULL;

static settings_info_t current_settings = {
    .performance_mode = PERFORMANCE_MEDIUM,
    .auto_fan_control = true,
    .fan_speed_percent = 50,
    .brightness_percent = 100
};

static lv_obj_t* create_settings_button(lv_obj_t* parent, const char* text, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, active ? COLOR_BACKGROUND : COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
    
    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

static lv_obj_t* create_bottom_nav_btn(lv_obj_t* parent, const char* symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 60, 50);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : lv_color_hex(0x4A5F6F), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_BACKGROUND : COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
    
    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

static void update_performance_buttons(void)
{
    if(!performance_low_btn || !performance_medium_btn || !performance_high_btn) return;
    
    lv_obj_set_style_bg_color(performance_low_btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_border_opa(performance_low_btn, LV_OPA_30, 0);
    lv_obj_t * low_label = lv_obj_get_child(performance_low_btn, 0);
    if(low_label) lv_obj_set_style_text_color(low_label, COLOR_ACCENT, 0);
    
    lv_obj_set_style_bg_color(performance_medium_btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_border_opa(performance_medium_btn, LV_OPA_30, 0);
    lv_obj_t * medium_label = lv_obj_get_child(performance_medium_btn, 0);
    if(medium_label) lv_obj_set_style_text_color(medium_label, COLOR_ACCENT, 0);
    
    lv_obj_set_style_bg_color(performance_high_btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_border_opa(performance_high_btn, LV_OPA_30, 0);
    lv_obj_t * high_label = lv_obj_get_child(performance_high_btn, 0);
    if(high_label) lv_obj_set_style_text_color(high_label, COLOR_ACCENT, 0);
    
    lv_obj_t * active_btn = NULL;
    lv_obj_t * active_label = NULL;
    
    switch(current_settings.performance_mode) {
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
    
    if(active_btn && active_label) {
        lv_obj_set_style_bg_color(active_btn, COLOR_ACCENT, 0);
        lv_obj_set_style_border_opa(active_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(active_label, COLOR_BACKGROUND, 0);
    }
}

static void update_fan_controls(void)
{
    if(!fan_slider || !fan_value_label) return;
    
    if(current_settings.auto_fan_control) {
        lv_obj_add_flag(fan_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(fan_value_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(fan_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(fan_value_label, LV_OBJ_FLAG_HIDDEN);
        
        lv_slider_set_value(fan_slider, current_settings.fan_speed_percent, LV_ANIM_OFF);
        char fan_text[16];
        snprintf(fan_text, sizeof(fan_text), "%d%%", current_settings.fan_speed_percent);
        lv_label_set_text(fan_value_label, fan_text);
    }
}

void settings_screen_create(void)
{
    if(settings_screen != NULL) {
        return;
    }
    
    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, 0);
    
    lv_obj_t * main_cont = lv_obj_create(settings_screen);
    lv_obj_set_size(main_cont, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 110);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(main_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(main_cont, 2, 0);
    lv_obj_set_style_border_color(main_cont, lv_color_hex(0x4A5F6F), 0);
    lv_obj_set_style_border_opa(main_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(main_cont, 12, 0);
    lv_obj_set_style_pad_all(main_cont, 20, 0);
    
    lv_obj_t * title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "SETTINGS");
    lv_obj_set_style_text_color(title_label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t * perf_section = lv_obj_create(main_cont);
    lv_obj_set_size(perf_section, 700, 120);
    lv_obj_align(perf_section, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(perf_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(perf_section, 0, 0);
    lv_obj_set_style_pad_all(perf_section, 10, 0);
    
    lv_obj_t * perf_title = lv_label_create(perf_section);
    lv_label_set_text(perf_title, "Performance Mode:");
    lv_obj_set_style_text_color(perf_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(perf_title, &lv_font_montserrat_18, 0);
    lv_obj_align(perf_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t * perf_btn_cont = lv_obj_create(perf_section);
    lv_obj_set_size(perf_btn_cont, 600, 60);
    lv_obj_align(perf_btn_cont, LV_ALIGN_TOP_LEFT, 0, 35);
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
    
    lv_obj_t * fan_section = lv_obj_create(main_cont);
    lv_obj_set_size(fan_section, 700, 165);
    lv_obj_align(fan_section, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_bg_opa(fan_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fan_section, 0, 0);
    lv_obj_set_style_pad_all(fan_section, 0, 0);
    
    lv_obj_t * fan_title = lv_label_create(fan_section);
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
    
    if(current_settings.auto_fan_control) {
        lv_obj_add_state(auto_fan_checkbox, LV_STATE_CHECKED);
    }
    
    fan_slider = lv_slider_create(fan_section);
    lv_obj_set_size(fan_slider, 400, 20);
    lv_obj_align(fan_slider, LV_ALIGN_TOP_LEFT, 0, 80);
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
    lv_obj_align(fan_value_label, LV_ALIGN_TOP_LEFT, 420, 75);
    
    fan_save_btn = create_settings_button(fan_section, "SAVE", settings_fan_save_clicked, false);
    lv_obj_set_size(fan_save_btn, 120, 40);
    lv_obj_align(fan_save_btn, LV_ALIGN_TOP_LEFT, 0, 120);
    
    update_fan_controls();
    
    lv_obj_t * brightness_section = lv_obj_create(main_cont);
    lv_obj_set_size(brightness_section, 700, 100);
    lv_obj_align(brightness_section, LV_ALIGN_TOP_MID, 0, 325);
    lv_obj_set_style_bg_opa(brightness_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(brightness_section, 0, 0);
    lv_obj_set_style_pad_all(brightness_section, 0, 0);
    
    lv_obj_t * brightness_title = lv_label_create(brightness_section);
    lv_label_set_text(brightness_title, "Screen Brightness:");
    lv_obj_set_style_text_color(brightness_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(brightness_title, &lv_font_montserrat_18, 0);
    lv_obj_align(brightness_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    brightness_slider = lv_slider_create(brightness_section);
    lv_obj_set_size(brightness_slider, 400, 20);
    lv_obj_align(brightness_slider, LV_ALIGN_TOP_LEFT, 0, 35);
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
    lv_obj_set_style_text_font(brightness_value_label, &lv_font_montserrat_16, 0);
    lv_obj_align(brightness_value_label, LV_ALIGN_TOP_LEFT, 420, 30);
    
    lv_obj_t * bottom_nav = lv_obj_create(settings_screen);
    lv_obj_set_size(bottom_nav, SCREEN_WIDTH, 70);
    lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_nav, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_nav, 0, 0);
    lv_obj_set_style_radius(bottom_nav, 0, 0);
    lv_obj_set_style_pad_all(bottom_nav, 10, 0);
    lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, settings_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, settings_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, NULL, true);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, settings_night_clicked, false);
}

void settings_screen_destroy(void)
{
    if(settings_screen) {
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
    }
}

lv_obj_t* settings_get_screen(void)
{
    return settings_screen;
}

void settings_update_info(const settings_info_t* info)
{
    if(info) {
        current_settings = *info;
        update_performance_buttons();
        update_fan_controls();
        
        if(auto_fan_checkbox) {
            if(current_settings.auto_fan_control) {
                lv_obj_add_state(auto_fan_checkbox, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(auto_fan_checkbox, LV_STATE_CHECKED);
            }
        }
        
        if(brightness_slider) {
            lv_slider_set_value(brightness_slider, current_settings.brightness_percent, LV_ANIM_OFF);
        }
        if(brightness_value_label) {
            char brightness_text[16];
            snprintf(brightness_text, sizeof(brightness_text), "%d%%", current_settings.brightness_percent);
            lv_label_set_text(brightness_value_label, brightness_text);
        }
    }
}

void settings_performance_low_clicked(lv_event_t * e)
{
    current_settings.performance_mode = PERFORMANCE_LOW;
    update_performance_buttons();
    printf("Performance mode set to LOW\n");
    
    BAP_send_frequency_setting(575.0f);
    BAP_send_asic_voltage(1160.0f);
}

void settings_performance_medium_clicked(lv_event_t * e)
{
    current_settings.performance_mode = PERFORMANCE_MEDIUM;
    update_performance_buttons();
    printf("Performance mode set to MEDIUM\n");
    
    BAP_send_frequency_setting(600.0f);
    BAP_send_asic_voltage(1200.0f);
}

void settings_performance_high_clicked(lv_event_t * e)
{
    current_settings.performance_mode = PERFORMANCE_HIGH;
    update_performance_buttons();
    printf("Performance mode set to HIGH\n");
    
    BAP_send_frequency_setting(655.0f);
    BAP_send_asic_voltage(1200.0f);
}

void settings_auto_fan_toggled(lv_event_t * e)
{
    lv_obj_t * checkbox = lv_event_get_target(e);
    current_settings.auto_fan_control = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    update_fan_controls();
    printf("Auto fan control: %s\n", current_settings.auto_fan_control ? "ON" : "OFF");
}

void settings_fan_slider_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    current_settings.fan_speed_percent = lv_slider_get_value(slider);
    
    if(fan_value_label) {
        char fan_text[16];
        snprintf(fan_text, sizeof(fan_text), "%d%%", current_settings.fan_speed_percent);
        lv_label_set_text(fan_value_label, fan_text);
    }
    
    printf("Fan speed set to: %d%%\n", current_settings.fan_speed_percent);
}

void settings_fan_save_clicked(lv_event_t * e)
{
    printf("Saving fan settings - Auto: %s, Speed: %d%%\n", 
           current_settings.auto_fan_control ? "ON" : "OFF",
           current_settings.fan_speed_percent);
    
    if(current_settings.auto_fan_control) {
        BAP_send_automatic_fan_control(true);
        printf("Sending auto fan control command\n");
    } else {
        BAP_send_fan_speed(current_settings.fan_speed_percent);
        printf("Sending manual fan speed: %d%%\n", current_settings.fan_speed_percent);
    }
}

void settings_home_clicked(lv_event_t * e)
{
    home_screen_create();
    lv_scr_load(home_get_screen());
    settings_screen_destroy();
}

void settings_wifi_clicked(lv_event_t * e)
{
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    settings_screen_destroy();
}

void settings_brightness_slider_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    current_settings.brightness_percent = lv_slider_get_value(slider);
    
    if(brightness_value_label) {
        char brightness_text[16];
        snprintf(brightness_text, sizeof(brightness_text), "%d%%", current_settings.brightness_percent);
        lv_label_set_text(brightness_value_label, brightness_text);
    }
    
    // Apply brightness change immediately
    lcd_backlight_set_brightness(current_settings.brightness_percent);
    
    printf("Screen brightness set to: %d%%\n", current_settings.brightness_percent);
}

void settings_night_clicked(lv_event_t * e)
{
    // Set brightness to 5% for night mode
    extern esp_err_t lcd_backlight_set_brightness(uint8_t brightness_percent);
    lcd_backlight_set_brightness(5);
    // Navigate to night mode screen
    night_screen_create();
    lv_scr_load(night_get_screen());
    settings_screen_destroy();
}