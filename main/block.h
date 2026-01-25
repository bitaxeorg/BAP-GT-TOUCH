#ifndef BLOCK_H
#define BLOCK_H

#include "lvgl.h"

void block_screen_create(void);
void block_screen_destroy(void);
lv_obj_t *block_get_screen(void);
void block_update_height(const char *height);

void block_home_clicked(lv_event_t *e);
void block_clock_clicked(lv_event_t *e);
void block_price_clicked(lv_event_t *e);
void block_mempool_clicked(lv_event_t *e);
void block_wifi_clicked(lv_event_t *e);
void block_settings_clicked(lv_event_t *e);
void block_night_clicked(lv_event_t *e);

#endif // BLOCK_H
