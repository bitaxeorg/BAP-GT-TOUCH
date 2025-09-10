#ifndef NIGHT_H
#define NIGHT_H

#include "lvgl.h"

// Function declarations
void night_screen_create(void);
void night_screen_destroy(void);
lv_obj_t* night_get_screen(void);
void night_update_hashrate(const char* hashrate);

// Event handlers
void night_home_clicked(lv_event_t * e);
void night_wifi_clicked(lv_event_t * e);
void night_settings_clicked(lv_event_t * e);

#endif // NIGHT_H