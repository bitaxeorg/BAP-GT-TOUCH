#ifndef PRICE_H
#define PRICE_H

#include "lvgl.h"

void price_screen_create(void);
void price_screen_destroy(void);
lv_obj_t *price_get_screen(void);

void price_home_clicked(lv_event_t *e);
void price_block_clicked(lv_event_t *e);
void price_clock_clicked(lv_event_t *e);
void price_mempool_clicked(lv_event_t *e);
void price_wifi_clicked(lv_event_t *e);
void price_settings_clicked(lv_event_t *e);
void price_night_clicked(lv_event_t *e);

#endif // PRICE_H
