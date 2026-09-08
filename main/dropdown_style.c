#include "dropdown_style.h"

void dropdown_style_apply(lv_obj_t *dropdown, const lv_font_t *font,
                          lv_color_t background, lv_color_t foreground,
                          lv_color_t accent, lv_color_t on_accent)
{
    lv_obj_set_style_bg_color(dropdown, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dropdown, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dropdown, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(dropdown, accent, LV_PART_MAIN);
    lv_obj_set_style_border_opa(dropdown, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(dropdown, 8, LV_PART_MAIN);
    lv_obj_set_style_text_color(dropdown, foreground, LV_PART_MAIN);
    lv_obj_set_style_text_font(dropdown, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(dropdown, accent, LV_PART_INDICATOR);
    lv_obj_set_style_outline_color(dropdown, accent, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(dropdown, accent, LV_STATE_EDITED);
    lv_obj_set_style_color_filter_opa(dropdown, LV_OPA_TRANSP, LV_STATE_PRESSED);

    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if (!list) return;
    lv_obj_set_style_bg_color(list, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(list, accent, LV_PART_MAIN);
    lv_obj_set_style_radius(list, 8, LV_PART_MAIN);
    lv_obj_set_style_text_color(list, foreground, LV_PART_MAIN);
    lv_obj_set_style_text_font(list, font, LV_PART_MAIN);

    const lv_state_t states[] = {LV_STATE_DEFAULT, LV_STATE_CHECKED,
                                LV_STATE_PRESSED, LV_STATE_CHECKED | LV_STATE_PRESSED};
    for (unsigned i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        lv_state_t state = states[i];
        lv_style_selector_t selector = LV_PART_SELECTED | state;
        bool checked = (state & LV_STATE_CHECKED) != 0;
        bool pressed = (state & LV_STATE_PRESSED) != 0;
        lv_obj_set_style_bg_color(list, checked ? accent : background, selector);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, selector);
        lv_obj_set_style_bg_grad_dir(list, LV_GRAD_DIR_NONE, selector);
        lv_obj_set_style_text_color(list, checked ? on_accent : foreground, selector);
        lv_obj_set_style_text_font(list, font, selector);
        lv_obj_set_style_border_width(list, pressed ? 2 : 0, selector);
        lv_obj_set_style_border_color(list, checked ? on_accent : accent, selector);
        lv_obj_set_style_border_opa(list, LV_OPA_COVER, selector);
        lv_obj_set_style_color_filter_opa(list, LV_OPA_TRANSP, selector);
    }
}
