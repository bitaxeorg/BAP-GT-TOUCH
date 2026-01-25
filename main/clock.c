#include "clock.h"
#include "home.h"
#include "block.h"
#include "wifi.h"
#include "settings.h"
#include "night.h"
#include "price.h"
#include "mempool.h"
#include "custom_fonts.h"
#include "esp_timer.h"
#include "lwip/apps/sntp.h"
#include <time.h>

static lv_obj_t *clock_screen = NULL;
static lv_obj_t *clock_time_cont = NULL;
static lv_obj_t *clock_time_label = NULL;
static lv_obj_t *clock_ampm_label = NULL;
static lv_obj_t *clock_title_label = NULL;
static lv_timer_t *clock_timer = NULL;

static char current_time_text[16] = "--:--";
static char current_ampm_text[4] = "--";

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active);
static void clock_start_sntp(void);
static void clock_update_time_text(void);
static void clock_timer_cb(lv_timer_t *timer);

void clock_screen_create(void)
{
    if (clock_screen != NULL)
    {
        return;
    }

    clock_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(clock_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(clock_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(clock_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(clock_screen, LV_SCROLLBAR_MODE_OFF);

    clock_title_label = lv_label_create(clock_screen);
    lv_label_set_text(clock_title_label, "CURRENT TIME");
    lv_obj_set_style_text_color(clock_title_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(clock_title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(clock_title_label, LV_ALIGN_TOP_MID, 0, 30);

    clock_time_cont = lv_obj_create(clock_screen);
    lv_obj_set_size(clock_time_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(clock_time_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clock_time_cont, 0, 0);
    lv_obj_set_style_pad_all(clock_time_cont, 0, 0);
    lv_obj_set_style_pad_column(clock_time_cont, 10, 0);
    lv_obj_set_flex_flow(clock_time_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clock_time_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(clock_time_cont, LV_ALIGN_CENTER, 0, -10);

    clock_time_label = lv_label_create(clock_time_cont);
    lv_label_set_text(clock_time_label, current_time_text);
    lv_obj_set_style_text_color(clock_time_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(clock_time_label, &montserrat_140, 0);
    lv_obj_set_style_text_letter_space(clock_time_label, 2, 0);

    clock_ampm_label = lv_label_create(clock_time_cont);
    lv_label_set_text(clock_ampm_label, current_ampm_text);
    lv_obj_set_style_text_color(clock_ampm_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_opa(clock_ampm_label, (lv_opa_t)192, 0);
    lv_obj_set_style_text_font(clock_ampm_label, &lv_font_montserrat_48, 0);

    lv_obj_t *bottom_nav = lv_obj_create(clock_screen);
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

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, clock_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_GPS, clock_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, clock_mempool_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LOOP, NULL, true);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_BARS, clock_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, clock_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, clock_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, clock_night_clicked, false);

    clock_start_sntp();
    clock_update_time_text();
    clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
}

void clock_screen_destroy(void)
{
    if (clock_screen)
    {
        if (clock_timer)
        {
            lv_timer_del(clock_timer);
            clock_timer = NULL;
        }
        lv_obj_del(clock_screen);
        clock_screen = NULL;
        clock_time_cont = NULL;
        clock_time_label = NULL;
        clock_ampm_label = NULL;
        clock_title_label = NULL;
    }
}

lv_obj_t *clock_get_screen(void)
{
    return clock_screen;
}

static void clock_update_time_text(void)
{
    time_t now = time(NULL);
    if (now < 946684800)
    {
        int64_t uptime_us = esp_timer_get_time();
        int32_t uptime_sec = (int32_t)(uptime_us / 1000000);
        int32_t hours = (uptime_sec / 3600) % 24;
        int32_t minutes = (uptime_sec / 60) % 60;
        int32_t hour12 = hours % 12;
        if (hour12 == 0)
        {
            hour12 = 12;
        }
        const char *ampm = (hours < 12) ? "AM" : "PM";
        lv_snprintf(current_time_text, sizeof(current_time_text), "%02d:%02d", (int)hour12, (int)minutes);
        lv_snprintf(current_ampm_text, sizeof(current_ampm_text), "%s", ampm);
    }
    else
    {
        struct tm time_info;
        localtime_r(&now, &time_info);
        int hour12 = time_info.tm_hour % 12;
        if (hour12 == 0)
        {
            hour12 = 12;
        }
        const char *ampm = (time_info.tm_hour < 12) ? "AM" : "PM";
        lv_snprintf(current_time_text, sizeof(current_time_text), "%02d:%02d",
                    hour12, time_info.tm_min);
        lv_snprintf(current_ampm_text, sizeof(current_ampm_text), "%s", ampm);
    }

    if (clock_time_label)
    {
        lv_label_set_text(clock_time_label, current_time_text);
    }
    if (clock_ampm_label)
    {
        lv_label_set_text(clock_ampm_label, current_ampm_text);
    }
}

static void clock_start_sntp(void)
{
    static bool sntp_started = false;
    if (sntp_started || sntp_enabled())
    {
        sntp_started = true;
        return;
    }

    sntp_started = true;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void clock_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    clock_update_time_text();
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

void clock_home_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    home_screen_create();
    lv_scr_load(home_get_screen());
    clock_screen_destroy();
}

void clock_block_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    block_screen_create();
    lv_scr_load(block_get_screen());
    clock_screen_destroy();
}

void clock_mempool_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    clock_screen_destroy();
}

void clock_price_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    price_screen_create();
    lv_scr_load(price_get_screen());
    clock_screen_destroy();
}

void clock_wifi_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    clock_screen_destroy();
}

void clock_settings_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    clock_screen_destroy();
}

void clock_night_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    night_screen_create();
    lv_scr_load(night_get_screen());
    clock_screen_destroy();
}
