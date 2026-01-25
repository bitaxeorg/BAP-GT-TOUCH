#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "lvgl.h"

void mempool_screen_create(void);
void mempool_screen_destroy(void);
lv_obj_t *mempool_get_screen(void);

void mempool_home_clicked(lv_event_t *e);
void mempool_block_clicked(lv_event_t *e);
void mempool_clock_clicked(lv_event_t *e);
void mempool_price_clicked(lv_event_t *e);
void mempool_wifi_clicked(lv_event_t *e);
void mempool_settings_clicked(lv_event_t *e);
void mempool_night_clicked(lv_event_t *e);

#endif // MEMPOOL_H
