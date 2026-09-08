#ifndef DROPDOWN_STYLE_H
#define DROPDOWN_STYLE_H

#include "lvgl.h"

void dropdown_style_apply(lv_obj_t *dropdown, const lv_font_t *font,
                          lv_color_t background, lv_color_t foreground,
                          lv_color_t accent, lv_color_t on_accent);

#endif
