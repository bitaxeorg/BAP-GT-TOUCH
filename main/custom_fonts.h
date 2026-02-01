#ifndef CUSTOM_FONTS_H
#define CUSTOM_FONTS_H

#include "lvgl.h" 

// Declare all your custom fonts here
LV_FONT_DECLARE(angelwish_96);
LV_FONT_DECLARE(untyped_96);
LV_FONT_DECLARE(Nevan_RUS_96);
LV_FONT_DECLARE(montserrat_120);
LV_FONT_DECLARE(montserrat_140);
LV_FONT_DECLARE(fa_clock_18);
// LV_FONT_DECLARE(angelwish_48);
// LV_FONT_DECLARE(angelwish_24);

// Font Awesome icons
#define FA_CLOCK "\xEF\x80\x97"  // Unicode 0xF017 (fa-clock)

// Custom images
LV_IMG_DECLARE(clock_solid_full);

#endif // CUSTOM_FONTS_H
