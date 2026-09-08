#ifndef THEME_H
#define THEME_H

#include "esp_err.h"
#include "lvgl.h"
#include "theme_palette.h"

void theme_initialize(void);
accent_theme_t theme_get_accent(void);
esp_err_t theme_set_accent(accent_theme_t theme);
lv_color_t theme_get_accent_color(void);
lv_color_t theme_get_text_on_accent_color(void);

#endif // THEME_H
