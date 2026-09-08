#ifndef SETTINGS_H
#define SETTINGS_H

#include "lvgl.h"

typedef enum {
    PERFORMANCE_LOW = 0,
    PERFORMANCE_MEDIUM,
    PERFORMANCE_HIGH
} performance_mode_t;

typedef enum {
    STARTUP_PAGE_HOME = 0,
    STARTUP_PAGE_PRICE,
    STARTUP_PAGE_BLOCKS,
    STARTUP_PAGE_MEMPOOL,
    STARTUP_PAGE_CLOCK,
    STARTUP_PAGE_HASHRATE,
    STARTUP_PAGE_COUNT
} startup_page_t;

typedef struct {
    performance_mode_t performance_mode;
    bool auto_fan_control;
    int fan_speed_percent;  // 0-100%
    int brightness_percent; // 0-100%
} settings_info_t;

void settings_screen_create(void);
void settings_initialize(void);
void settings_screen_destroy(void);
lv_obj_t* settings_get_screen(void);
void settings_update_info(const settings_info_t* info);
startup_page_t settings_get_startup_page(void);

void settings_performance_low_clicked(lv_event_t * e);
void settings_performance_medium_clicked(lv_event_t * e);
void settings_performance_high_clicked(lv_event_t * e);
void settings_auto_fan_toggled(lv_event_t * e);
void settings_fan_slider_changed(lv_event_t * e);
void settings_fan_save_clicked(lv_event_t * e);
void settings_brightness_slider_changed(lv_event_t * e);
void settings_timezone_changed(lv_event_t * e);
void settings_startup_page_changed(lv_event_t * e);
void settings_home_clicked(lv_event_t * e);
void settings_block_clicked(lv_event_t * e);
void settings_clock_clicked(lv_event_t * e);
void settings_price_clicked(lv_event_t * e);
void settings_mempool_clicked(lv_event_t * e);
void settings_wifi_clicked(lv_event_t * e);
void settings_night_clicked(lv_event_t * e);

#endif // SETTINGS_H
