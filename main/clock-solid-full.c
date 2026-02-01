#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "../components/lvgl__lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_CLOCK_SOLID_FULL
#define LV_ATTRIBUTE_IMG_CLOCK_SOLID_FULL
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CLOCK_SOLID_FULL uint8_t clock_solid_full_map[] = {
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xc0, 
};

const lv_img_dsc_t clock_solid_full = {
  .header.cf = LV_IMG_CF_ALPHA_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 18,
  .header.h = 18,
  .data_size = 54,
  .data = clock_solid_full_map,
};
