/*******************************************************************************
 * Size: 18 px
 * Bpp: 4
 * Opts: --bpp 4 --size 18 --font FontAwesome5-Solid+Brands+Regular.woff -r 0xF017 --format lvgl -o /BAP-GT-TOUCH/main/fa_clock_18.c --force-fast-kern-format
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FA_CLOCK_18
#define FA_CLOCK_18 1
#endif

#if FA_CLOCK_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F017 "" */
    0x0, 0xf8, 0xd5, 0x90, 0x40, 0x3f, 0xc3, 0x3f,
    0x95, 0x37, 0xd8, 0x80, 0x1f, 0x27, 0x30, 0x7,
    0x8e, 0xe4, 0x3, 0x92, 0xc0, 0x3f, 0xcd, 0x40,
    0x11, 0x58, 0x7, 0x7f, 0x9c, 0x3, 0x2b, 0x80,
    0x20, 0x3, 0x84, 0x3, 0xf4, 0x8, 0x94, 0x3,
    0xff, 0x86, 0xc8, 0x1, 0xff, 0xc4, 0xc2, 0x0,
    0xff, 0xe2, 0x37, 0x80, 0x7f, 0x58, 0x80, 0x71,
    0xf0, 0x7, 0x84, 0x80, 0xfd, 0x0, 0x31, 0x28,
    0x7, 0xda, 0xa0, 0x3e, 0x1, 0x98, 0xc4, 0x3,
    0xea, 0xa1, 0x50, 0x6, 0xf0, 0x90, 0xf, 0xcb,
    0xa2, 0x1, 0x12, 0x83, 0x98, 0x7, 0xff, 0x7,
    0x80, 0x2d, 0x20, 0xf, 0xfa, 0x8c, 0x2, 0x1d,
    0x50, 0xf, 0xc5, 0x8a, 0x1, 0xea, 0xc6, 0x20,
    0x0, 0xa5, 0x69, 0x80, 0x7e, 0x39, 0xdf, 0xf7,
    0x5a, 0x80, 0x70
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 288, .box_w = 18, .box_h = 19, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 61463, .range_length = 1, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 1,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t fa_clock_18 = {
#else
lv_font_t fa_clock_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 19,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -6,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if FA_CLOCK_18*/

