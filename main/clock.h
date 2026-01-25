#ifndef CLOCK_H
#define CLOCK_H

#include "lvgl.h"

void clock_screen_create(void);
void clock_screen_destroy(void);
lv_obj_t *clock_get_screen(void);

void clock_home_clicked(lv_event_t *e);
void clock_block_clicked(lv_event_t *e);
void clock_price_clicked(lv_event_t *e);
void clock_mempool_clicked(lv_event_t *e);
void clock_wifi_clicked(lv_event_t *e);
void clock_settings_clicked(lv_event_t *e);
void clock_night_clicked(lv_event_t *e);

#endif // CLOCK_H
