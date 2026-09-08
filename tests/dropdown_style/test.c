#include "dropdown_style.h"
#include "theme_palette.h"
#include <assert.h>
#include <stdio.h>

static lv_color_t framebuffer[800 * 480];
static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *colors)
{
    (void)area; (void)colors; lv_disp_flush_ready(driver);
}

int main(void)
{
    lv_init();
    static lv_disp_draw_buf_t buffer;
    static lv_disp_drv_t driver;
    lv_disp_draw_buf_init(&buffer, framebuffer, NULL, 800 * 480);
    lv_disp_drv_init(&driver);
    driver.hor_res = 800; driver.ver_res = 480; driver.draw_buf = &buffer;
    driver.direct_mode = 1; driver.flush_cb = flush; lv_disp_drv_register(&driver);
    const lv_color_t background = lv_color_hex(0x0f1218);
    const lv_color_t foreground = lv_color_white();
    const lv_state_t states[] = {LV_STATE_DEFAULT, LV_STATE_CHECKED,
                                LV_STATE_PRESSED, LV_STATE_CHECKED | LV_STATE_PRESSED};
    for (int i = 0; i < ACCENT_THEME_COUNT; ++i) {
        lv_color_t accent = lv_color_hex(theme_palette_accent_hex(i));
        lv_color_t on_accent = lv_color_hex(theme_palette_text_hex(i));
        lv_obj_t *dd = lv_dropdown_create(lv_scr_act());
        lv_dropdown_set_options(dd, "Current\nPress here\nOther");
        dropdown_style_apply(dd, &lv_font_montserrat_16, background, foreground, accent, on_accent);
        for (int reopen = 0; reopen < 2; ++reopen) {
            lv_dropdown_open(dd);
            lv_obj_t *list = lv_dropdown_get_list(dd);
            for (unsigned s = 0; s < sizeof(states) / sizeof(states[0]); ++s) {
                lv_obj_clear_state(list, LV_STATE_ANY); lv_obj_add_state(list, states[s]);
                bool checked = (states[s] & LV_STATE_CHECKED) != 0;
                bool pressed = (states[s] & LV_STATE_PRESSED) != 0;
                lv_draw_rect_dsc_t rect; lv_draw_rect_dsc_init(&rect);
                lv_obj_init_draw_rect_dsc(list, LV_PART_SELECTED, &rect);
                lv_draw_label_dsc_t text; lv_draw_label_dsc_init(&text);
                lv_obj_init_draw_label_dsc(list, LV_PART_SELECTED, &text);
                assert(rect.bg_color.full == (checked ? accent : background).full);
                assert(rect.bg_opa == LV_OPA_COVER);
                assert(text.color.full == (checked ? on_accent : foreground).full);
                assert(rect.border_width == (pressed ? 2 : 0));
                if (pressed) assert(rect.border_color.full == (checked ? on_accent : accent).full);
            }
            lv_obj_clear_state(list, LV_STATE_ANY);
            if (!reopen) lv_dropdown_close(dd);
        }
    }
    puts("PASS: 5 accents x 4 effective draw states x 2 openings");
    return 0;
}
