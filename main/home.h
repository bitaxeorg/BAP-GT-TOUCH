#ifndef HOME_H
#define HOME_H

#include "lvgl.h"

// Hardware information structure
typedef struct {
    char model[32];
    char chip[32];
    char efficiency[16];
    char fan_speed[16];
} hardware_info_t;

// Pool information structure
typedef struct {
    char url[128];
    char port[16];
    char worker_name[64];
    char status[32];
} pool_info_t;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Colors
#define COLOR_BACKGROUND    lv_color_hex(0x2B3A4A)
#define COLOR_CARD_BG       lv_color_hex(0x3C4F5F)
#define COLOR_ACCENT        lv_color_hex(0xF4D03F)
#define COLOR_RED           lv_color_hex(0xFF0000)
#define COLOR_TEXT_PRIMARY  lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_SECONDARY lv_color_hex(0xBDC3C7)

// Function declarations
void home_screen_create(void);
void home_screen_destroy(void);
void home_update_hashrate(const char* hashrate);
lv_obj_t* home_get_screen(void);

// Hardware data functions
void home_update_hardware_info(const hardware_info_t* hw_info);
void home_update_power(const char* power);
void update_efficiency_display(void);
void home_update_temperature(const char* temperature);
void home_update_device_model(const char* model);
void home_update_asic_model(const char* chip);
void home_update_fan_speed(const char* fan_rpm);
void home_update_shares(const char* shares);
void home_update_best_difficulty(const char* bd);

// Pool data functions
void home_update_pool_info(const pool_info_t* pool_info);

// Event handlers
void home_hardware_clicked(lv_event_t * e);
void home_pool_clicked(lv_event_t * e);
void home_settings_clicked(lv_event_t * e);
void home_night_clicked(lv_event_t * e);
void home_wifi_clicked(lv_event_t * e);

#endif // HOME_H