#include "home.h"
#include "wifi.h"
#include "settings.h"
#include "night.h"
#include "block.h"
#include "clock.h"
#include "price.h"
#include "mempool.h"
#include "stdio.h"
#include "string.h"
#include "custom_fonts.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "assets/temperature_icon.h"
#include "assets/fan.h"
#include "assets/star.h"

static lv_obj_t *home_screen = NULL;
static lv_obj_t *hashrate_label = NULL;
static lv_obj_t *hardware_popup = NULL;
static lv_obj_t *pool_popup = NULL;
static lv_obj_t *power_label = NULL;
static lv_obj_t *temperature_label = NULL;
static lv_obj_t *efficiency_label = NULL;
static lv_obj_t *fan_label = NULL;
static lv_obj_t *shares_label = NULL;
static lv_obj_t *bd_label = NULL;

static float current_power_watts = 0.0f;
static float current_hashrate_ghs = 0.0f;
static char current_hashrate_text[16] = "";
static char current_power_text[16] = "";
static char current_temperature_text[16] = "";
static char current_fan_text[16] = "";
static char current_shares_text[32] = "";
static char current_bd_text[32] = "";
static char current_efficiency_text[32] = "";

static hardware_info_t current_hardware_info = {
    .model = "loading...",
    .chip = "loading...",
    .fan_speed = "loading..."};

static pool_info_t current_pool_info = {
    .url = "loading...",
    .port = "loading...",
    .worker_name = "bitaxe_001",
    .status = "Connected"};

static void hardware_popup_close_clicked(lv_event_t *e);
static void pool_popup_close_clicked(lv_event_t *e);
static void apply_cached_home_values(void);

static lv_obj_t *create_nav_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 220, 60);
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_TEXT_ON_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

static void create_hardware_popup(void)
{
    if (hardware_popup != NULL)
    {
        return;
    }

    hardware_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(hardware_popup, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(hardware_popup, 0, 0);
    lv_obj_set_style_bg_color(hardware_popup, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(hardware_popup, LV_OPA_70, 0);
    lv_obj_set_style_border_width(hardware_popup, 0, 0);
    lv_obj_set_style_pad_all(hardware_popup, 0, 0);

    lv_obj_t *popup_cont = lv_obj_create(hardware_popup);
    lv_obj_set_size(popup_cont, 500, 350);
    lv_obj_center(popup_cont);
    lv_obj_set_style_bg_color(popup_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(popup_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(popup_cont, 2, 0);
    lv_obj_set_style_border_color(popup_cont, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(popup_cont, LV_OPA_70, 0);
    lv_obj_set_style_radius(popup_cont, 12, 0);
    lv_obj_set_style_pad_all(popup_cont, 24, 0);

    lv_obj_t *title = lv_label_create(popup_cont);
    lv_label_set_text(title, "Hardware Information");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *close_btn = lv_btn_create(popup_cont);
    lv_obj_set_size(close_btn, 40, 40);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(close_btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 2, 0);
    lv_obj_set_style_border_color(close_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(close_btn, 20, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, COLOR_ACCENT, 0);
    lv_obj_center(close_label);

    lv_obj_add_event_cb(close_btn, hardware_popup_close_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *info_cont = lv_obj_create(popup_cont);
    lv_obj_set_size(info_cont, 460, 250);
    lv_obj_align(info_cont, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_cont, 0, 0);
    lv_obj_set_style_pad_all(info_cont, 10, 0);

    char buffer[64];
    const char *labels[] = {"Model", "Chip", "Fan Speed"};
    const char *values[] = {
        current_hardware_info.model,
        current_hardware_info.chip,
        current_hardware_info.fan_speed};

    int y_offset = 0;
    for (int i = 0; i < 3; i++)
    {
        lv_obj_t *info_label = lv_label_create(info_cont);
        snprintf(buffer, sizeof(buffer), "%s: %s", labels[i], values[i]);
        lv_label_set_text(info_label, buffer);
        lv_obj_set_style_text_color(info_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(info_label, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(info_label, 0, y_offset);
        y_offset += 28;
    }
}

static void hardware_popup_close_clicked(lv_event_t *e)
{
    if (hardware_popup)
    {
        lv_obj_del(hardware_popup);
        hardware_popup = NULL;
    }
}

static void create_pool_popup(void)
{
    if (pool_popup != NULL)
    {
        return;
    }

    pool_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pool_popup, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(pool_popup, 0, 0);
    lv_obj_set_style_bg_color(pool_popup, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(pool_popup, LV_OPA_70, 0);
    lv_obj_set_style_border_width(pool_popup, 0, 0);
    lv_obj_set_style_pad_all(pool_popup, 0, 0);

    lv_obj_t *popup_cont = lv_obj_create(pool_popup);
    lv_obj_set_size(popup_cont, 500, 300);
    lv_obj_center(popup_cont);
    lv_obj_set_style_bg_color(popup_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(popup_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(popup_cont, 2, 0);
    lv_obj_set_style_border_color(popup_cont, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(popup_cont, LV_OPA_70, 0);
    lv_obj_set_style_radius(popup_cont, 12, 0);
    lv_obj_set_style_pad_all(popup_cont, 24, 0);

    lv_obj_t *title = lv_label_create(popup_cont);
    lv_label_set_text(title, "Pool Information");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *close_btn = lv_btn_create(popup_cont);
    lv_obj_set_size(close_btn, 40, 40);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(close_btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 2, 0);
    lv_obj_set_style_border_color(close_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(close_btn, 20, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, COLOR_ACCENT, 0);
    lv_obj_center(close_label);

    lv_obj_add_event_cb(close_btn, pool_popup_close_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *info_cont = lv_obj_create(popup_cont);
    lv_obj_set_size(info_cont, 460, 200);
    lv_obj_align(info_cont, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_cont, 0, 0);
    lv_obj_set_style_pad_all(info_cont, 10, 0);

    char buffer[128];
    const char *labels[] = {"URL", "Port", "User", "Status"};
    const char *values[] = {
        current_pool_info.url,
        current_pool_info.port,
        current_pool_info.worker_name,
        current_pool_info.status};

    int y_offset = 0;
    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *info_label = lv_label_create(info_cont);
        snprintf(buffer, sizeof(buffer), "%s: %s", labels[i], values[i]);
        lv_label_set_text(info_label, buffer);
        lv_obj_set_style_text_color(info_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(info_label, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(info_label, 0, y_offset);
        y_offset += 35;
    }
}

static void pool_popup_close_clicked(lv_event_t *e)
{
    if (pool_popup)
    {
        lv_obj_del(pool_popup);
        pool_popup = NULL;
    }
}

static void apply_cached_home_values(void)
{
    if (hashrate_label)
    {
        lv_label_set_text(hashrate_label, strlen(current_hashrate_text) ? current_hashrate_text : "loading...");
    }
    if (power_label)
    {
        lv_label_set_text(power_label, strlen(current_power_text) ? current_power_text : "loading...");
    }
    if (temperature_label)
    {
        lv_label_set_text(temperature_label, strlen(current_temperature_text) ? current_temperature_text : "loading...");
    }
    if (fan_label)
    {
        lv_label_set_text(fan_label, strlen(current_fan_text) ? current_fan_text : "loading...");
    }
    if (shares_label)
    {
        lv_label_set_text(shares_label, strlen(current_shares_text) ? current_shares_text : "loading...");
    }
    if (bd_label)
    {
        lv_label_set_text(bd_label, strlen(current_bd_text) ? current_bd_text : "loading...");
    }
    if (efficiency_label)
    {
        lv_label_set_text(efficiency_label, strlen(current_efficiency_text) ? current_efficiency_text : "-- J/TH");
    }
}

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 56, 46);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

void home_screen_create(void)
{
    if (home_screen != NULL)
    {
        return;
    }

    home_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(home_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(home_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(home_screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *main_cont = lv_obj_create(home_screen);
    lv_obj_set_size(main_cont, SCREEN_WIDTH - 60, SCREEN_HEIGHT - 100);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(main_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_color(main_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_grad_dir(main_cont, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_border_width(main_cont, 1, 0);
    lv_obj_set_style_border_color(main_cont, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_cont, 14, 0);
    lv_obj_set_style_pad_all(main_cont, 16, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_style_shadow_opa(main_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_width(main_cont, 0, 0);
    lv_obj_set_style_outline_opa(main_cont, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t * title_label = lv_label_create(main_cont);
    // lv_label_set_text(title_label, "BITAXE STATS");
    // lv_obj_set_style_text_color(title_label, COLOR_ACCENT, 0);
    // lv_obj_set_style_text_font(title_label, &Nevan_RUS_96, 0);
    // lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *main_display_cont = lv_obj_create(main_cont);
    lv_obj_set_size(main_display_cont, 680, 110);
    lv_obj_align(main_display_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(main_display_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_display_cont, 0, 0);
    lv_obj_set_style_pad_all(main_display_cont, 0, 0);
    lv_obj_clear_flag(main_display_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *power_cont = lv_obj_create(main_display_cont);
    lv_obj_set_size(power_cont, 220, 110);
    lv_obj_align(power_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(power_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(power_cont, 0, 0);
    lv_obj_set_style_pad_all(power_cont, 0, 0);

    lv_obj_t *power_icon = lv_label_create(power_cont);
    lv_label_set_text(power_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(power_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(power_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(power_icon, LV_ALIGN_CENTER, 0, -24);

    power_label = lv_label_create(power_cont);
    lv_label_set_text(power_label, "loading...");
    lv_obj_set_style_text_color(power_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(power_label, &lv_font_montserrat_18, 0);
    lv_obj_align(power_label, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *hashrate_cont = lv_obj_create(main_display_cont);
    lv_obj_set_size(hashrate_cont, 220, 110);
    lv_obj_align(hashrate_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(hashrate_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hashrate_cont, 0, 0);
    lv_obj_set_style_pad_all(hashrate_cont, 0, 0);

    hashrate_label = lv_label_create(hashrate_cont);
    lv_label_set_text(hashrate_label, "loading...");
    lv_obj_set_style_text_color(hashrate_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(hashrate_label, &lv_font_montserrat_48, 0);
    lv_obj_align(hashrate_label, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *unit_label = lv_label_create(hashrate_cont);
    lv_label_set_text(unit_label, "GH/s");
    lv_obj_set_style_text_color(unit_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_20, 0);
    lv_obj_align(unit_label, LV_ALIGN_CENTER, -55, 34);

    efficiency_label = lv_label_create(hashrate_cont);
    lv_label_set_text(efficiency_label, "-- J/TH");
    lv_obj_set_style_text_color(efficiency_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(efficiency_label, &lv_font_montserrat_20, 0);
    lv_obj_align(efficiency_label, LV_ALIGN_CENTER, 55, 34);

    lv_obj_t *temp_cont = lv_obj_create(main_display_cont);
    lv_obj_set_size(temp_cont, 220, 110);
    lv_obj_align(temp_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(temp_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(temp_cont, 0, 0);
    lv_obj_set_style_pad_all(temp_cont, 0, 0);

    lv_obj_t *temp_icon = lv_img_create(temp_cont);
    lv_img_set_src(temp_icon, &temperature_icon);
    lv_obj_set_style_img_recolor(temp_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(temp_icon, LV_OPA_COVER, 0);
    lv_obj_align(temp_icon, LV_ALIGN_CENTER, 0, -24);

    temperature_label = lv_label_create(temp_cont);
    lv_label_set_text(temperature_label, "loading...");
    lv_obj_set_style_text_color(temperature_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(temperature_label, &lv_font_montserrat_18, 0);
    lv_obj_align(temperature_label, LV_ALIGN_CENTER, 0, 26);

    lv_obj_t *second_row_cont = lv_obj_create(main_cont);
    lv_obj_set_size(second_row_cont, 680, 100);
    lv_obj_align(second_row_cont, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_opa(second_row_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(second_row_cont, 0, 0);
    lv_obj_set_style_pad_all(second_row_cont, 0, 0);
    lv_obj_clear_flag(second_row_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bd_cont = lv_obj_create(second_row_cont);
    lv_obj_set_size(bd_cont, 220, 110);
    lv_obj_align(bd_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(bd_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bd_cont, 0, 0);
    lv_obj_set_style_pad_all(bd_cont, 0, 0);

    lv_obj_t *bd_icon = lv_img_create(bd_cont);
    lv_img_set_src(bd_icon, &star);
    lv_obj_set_style_img_recolor(bd_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(bd_icon, LV_OPA_COVER, 0);
    lv_obj_align(bd_icon, LV_ALIGN_CENTER, 0, -24);

    bd_label = lv_label_create(bd_cont);
    lv_label_set_text(bd_label, "loading...");
    lv_obj_set_style_text_color(bd_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(bd_label, &lv_font_montserrat_20, 0);
    lv_obj_align(bd_label, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *share_icon = lv_label_create(second_row_cont);
    lv_label_set_text(share_icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(share_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(share_icon, &lv_font_montserrat_32, 0);
    lv_obj_align(share_icon, LV_ALIGN_CENTER, -45, -32);

    lv_obj_t *share_reject_icon = lv_label_create(second_row_cont);
    lv_label_set_text(share_reject_icon, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_color(share_reject_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(share_reject_icon, &lv_font_montserrat_32, 0);
    lv_obj_align(share_reject_icon, LV_ALIGN_CENTER, 45, -32);

    shares_label = lv_label_create(second_row_cont);
    lv_label_set_text(shares_label, "loading...");
    lv_obj_set_style_text_color(shares_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(shares_label, &lv_font_montserrat_32, 0);
    lv_obj_align(shares_label, LV_ALIGN_CENTER, 0, 15);

    lv_obj_t *fan_cont = lv_obj_create(second_row_cont);
    lv_obj_set_size(fan_cont, 220, 110);
    lv_obj_align(fan_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(fan_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fan_cont, 0, 0);
    lv_obj_set_style_pad_all(fan_cont, 0, 0);

    lv_obj_t *fan_icon = lv_img_create(fan_cont);
    lv_img_set_src(fan_icon, &fan);
    lv_obj_set_style_img_recolor(fan_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(fan_icon, LV_OPA_COVER, 0);
    lv_obj_align(fan_icon, LV_ALIGN_CENTER, 0, -24);

    fan_label = lv_label_create(fan_cont);
    lv_label_set_text(fan_label, "loading...");
    lv_obj_set_style_text_color(fan_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(fan_label, &lv_font_montserrat_18, 0);
    lv_obj_align(fan_label, LV_ALIGN_CENTER, 0, 24);

    lv_obj_t *nav_cont = lv_obj_create(main_cont);
    lv_obj_set_size(nav_cont, 680, 70);
    lv_obj_align(nav_cont, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_opa(nav_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_cont, 0, 0);
    lv_obj_set_style_pad_all(nav_cont, 0, 0);
    lv_obj_clear_flag(nav_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(nav_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_nav_button(nav_cont, "Hardware", home_hardware_clicked);
    create_nav_button(nav_cont, "Pool", home_pool_clicked);

    lv_obj_t *bottom_nav = lv_obj_create(home_screen);
    lv_obj_set_size(bottom_nav, SCREEN_WIDTH, 64);
    lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_nav, COLOR_NAV_BG, 0);
    lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_nav, 0, 0);
    lv_obj_set_style_radius(bottom_nav, 0, 0);
    lv_obj_set_style_pad_all(bottom_nav, 8, 0);
    lv_obj_clear_flag(bottom_nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(bottom_nav, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, NULL, true);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_GPS, home_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, home_mempool_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LOOP, home_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", home_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, home_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, home_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, home_night_clicked, false);

    apply_cached_home_values();
}

void home_screen_destroy(void)
{
    if (home_screen)
    {
        lv_obj_del(home_screen);
        home_screen = NULL;
        hashrate_label = NULL;
        hardware_popup = NULL;
        pool_popup = NULL;
        power_label = NULL;
        temperature_label = NULL;
        efficiency_label = NULL;
        shares_label = NULL;
        fan_label = NULL;
        bd_label = NULL;
    }
}

void home_update_hashrate(const char *hashrate)
{
    if (!hashrate)
    {
        return;
    }

    strncpy(current_hashrate_text, hashrate, sizeof(current_hashrate_text) - 1);
    current_hashrate_text[sizeof(current_hashrate_text) - 1] = '\0';
    if (hashrate_label)
    {
        lv_label_set_text(hashrate_label, hashrate);
    }

    current_hashrate_ghs = atof(hashrate);

    update_efficiency_display();
}

lv_obj_t *home_get_screen(void)
{
    return home_screen;
}

void home_hardware_clicked(lv_event_t *e)
{
    create_hardware_popup();
}

void home_pool_clicked(lv_event_t *e)
{
    create_pool_popup();
}

void home_settings_clicked(lv_event_t *e)
{
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    home_screen_destroy();
}

void home_night_clicked(lv_event_t *e)
{
    // Navigate to night mode screen
    night_screen_create();
    lv_scr_load(night_get_screen());
    home_screen_destroy();
}

void home_wifi_clicked(lv_event_t *e)
{
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    home_screen_destroy();
}

void home_clock_clicked(lv_event_t *e)
{
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    home_screen_destroy();
}

void home_price_clicked(lv_event_t *e)
{
    price_screen_create();
    lv_scr_load(price_get_screen());
    home_screen_destroy();
}

void home_block_clicked(lv_event_t *e)
{
    block_screen_create();
    lv_scr_load(block_get_screen());
    home_screen_destroy();
}

void home_mempool_clicked(lv_event_t *e)
{
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    home_screen_destroy();
}

void home_update_hardware_info(const hardware_info_t *hw_info)
{
    if (hw_info)
    {
        memcpy(&current_hardware_info, hw_info, sizeof(hardware_info_t));
        if (hardware_popup)
        {
            lv_obj_del(hardware_popup);
            hardware_popup = NULL;
        }
    }
}

void home_update_power(const char *power)
{
    if (!power)
    {
        return;
    }

    char power_buffer[16];
    snprintf(power_buffer, sizeof(power_buffer), "%sW", power);
    strncpy(current_power_text, power_buffer, sizeof(current_power_text) - 1);
    current_power_text[sizeof(current_power_text) - 1] = '\0';

    if (power_label)
    {
        lv_label_set_text(power_label, power_buffer);
    }

    current_power_watts = atof(power);
    update_efficiency_display();
}

void update_efficiency_display(void)
{
    if (current_hashrate_ghs > 0.0f && current_power_watts > 0.0f)
    {
        float efficiency_j_th = current_power_watts / (current_hashrate_ghs / 1000.0f);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f J/TH", efficiency_j_th);
        strncpy(current_efficiency_text, buffer, sizeof(current_efficiency_text) - 1);
        current_efficiency_text[sizeof(current_efficiency_text) - 1] = '\0';
        if (efficiency_label)
        {
            lv_label_set_text(efficiency_label, buffer);
        }
    }
    else
    {
        strncpy(current_efficiency_text, "-- J/TH", sizeof(current_efficiency_text) - 1);
        current_efficiency_text[sizeof(current_efficiency_text) - 1] = '\0';
        if (efficiency_label)
        {
            lv_label_set_text(efficiency_label, "-- J/TH");
        }
    }
}
void home_update_temperature(const char *temperature)
{
    if (!temperature)
    {
        return;
    }

    char temp_buffer[16];
    snprintf(temp_buffer, sizeof(temp_buffer), "%s°C", temperature);
    strncpy(current_temperature_text, temp_buffer, sizeof(current_temperature_text) - 1);
    current_temperature_text[sizeof(current_temperature_text) - 1] = '\0';

    if (temperature_label)
    {
        lv_label_set_text(temperature_label, temp_buffer);
    }
}

void home_update_fan_speed(const char *fan_rpm)
{
    if (!fan_rpm)
    {
        return;
    }

    char fan_rpm_buffer[16];
    snprintf(fan_rpm_buffer, sizeof(fan_rpm_buffer), "%s RPM", fan_rpm);
    strncpy(current_fan_text, fan_rpm_buffer, sizeof(current_fan_text) - 1);
    current_fan_text[sizeof(current_fan_text) - 1] = '\0';

    if (fan_label)
    {
        lv_label_set_text(fan_label, fan_rpm_buffer);
    }
}

void home_update_pool_info(const pool_info_t *pool_info)
{
    if (pool_info)
    {
        if (strlen(pool_info->url) > 0)
        {
            strncpy(current_pool_info.url, pool_info->url, sizeof(current_pool_info.url) - 1);
            current_pool_info.url[sizeof(current_pool_info.url) - 1] = '\0';
        }
        if (strlen(pool_info->port) > 0)
        {
            strncpy(current_pool_info.port, pool_info->port, sizeof(current_pool_info.port) - 1);
            current_pool_info.port[sizeof(current_pool_info.port) - 1] = '\0';
        }
        if (strlen(pool_info->worker_name) > 0)
        {
            strncpy(current_pool_info.worker_name, pool_info->worker_name, sizeof(current_pool_info.worker_name) - 1);
            current_pool_info.worker_name[sizeof(current_pool_info.worker_name) - 1] = '\0';
        }
        if (strlen(pool_info->status) > 0)
        {
            strncpy(current_pool_info.status, pool_info->status, sizeof(current_pool_info.status) - 1);
            current_pool_info.status[sizeof(current_pool_info.status) - 1] = '\0';
        }

        if (pool_popup)
        {
            lv_obj_del(pool_popup);
            pool_popup = NULL;
        }
    }
}

void home_update_device_model(const char *model)
{
    if (model)
    {
        strncpy(current_hardware_info.model, model, sizeof(current_hardware_info.model) - 1);
        current_hardware_info.model[sizeof(current_hardware_info.model) - 1] = '\0';
        if (hardware_popup)
        {
            lv_obj_del(hardware_popup);
            hardware_popup = NULL;
        }
    }
}

void home_update_asic_model(const char *chip)
{
    if (chip)
    {
        strncpy(current_hardware_info.chip, chip, sizeof(current_hardware_info.chip) - 1);
        current_hardware_info.chip[sizeof(current_hardware_info.chip) - 1] = '\0';
        if (hardware_popup)
        {
            lv_obj_del(hardware_popup);
            hardware_popup = NULL;
        }
    }
}

void home_update_shares(const char *shares)
{
    if (!shares)
    {
        return;
    }

    char shares_buffer[32];
    snprintf(shares_buffer, sizeof(shares_buffer), "%s", shares);
    strncpy(current_shares_text, shares_buffer, sizeof(current_shares_text) - 1);
    current_shares_text[sizeof(current_shares_text) - 1] = '\0';

    if (shares_label)
    {
        lv_label_set_text(shares_label, shares_buffer);
    }
}

void home_update_best_difficulty(const char *bd)
{
    if (!bd)
    {
        return;
    }

    char bd_buffer[32];
    snprintf(bd_buffer, sizeof(bd_buffer), "%s", bd);
    strncpy(current_bd_text, bd_buffer, sizeof(current_bd_text) - 1);
    current_bd_text[sizeof(current_bd_text) - 1] = '\0';

    if (bd_label)
    {
        lv_label_set_text(bd_label, bd_buffer);
    }
}
