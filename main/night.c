#include "night.h"
#include "home.h"
#include "settings.h"
#include "wifi.h"
#include "block.h"
#include "clock.h"
#include "price.h"
#include "mempool.h"
#include "bap.h"
#include "custom_fonts.h"
#include "stdio.h"
#include "string.h"

#define COLOR_NIGHT_BG COLOR_BACKGROUND
#define COLOR_NIGHT_ACCENT COLOR_ACCENT
#define COLOR_NIGHT_TEXT COLOR_TEXT_PRIMARY
#define COLOR_NIGHT_CARD COLOR_CARD_BG

#define NIGHT_SCREEN_HASHRATE_POINTS 100

static lv_obj_t *night_screen = NULL;
static lv_obj_t *hashrate_label = NULL;
static lv_obj_t *unit_label = NULL;
static lv_obj_t *hashrate_chart = NULL;
static lv_chart_series_t *hashrate_series = NULL;

static float hashrate_history[NIGHT_SCREEN_HASHRATE_POINTS];
static int history_index = 0;
static int history_count = 0;
static bool first_data_received = false;
static char last_hashrate_text[16] = "0.00";

static void create_bottom_nav(void);
static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active);
static void apply_cached_hashrate(void);

void night_screen_create(void)
{
    if (night_screen != NULL)
    {
        return;
    }

    night_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(night_screen, COLOR_NIGHT_BG, 0);
    lv_obj_set_style_bg_opa(night_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(night_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(night_screen, LV_SCROLLBAR_MODE_OFF);

    // Large hashrate display at the top
    hashrate_label = lv_label_create(night_screen);
    lv_label_set_text(hashrate_label, "0.00");
    lv_obj_set_style_text_color(hashrate_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(hashrate_label, &lv_font_montserrat_48, 0);
    lv_obj_align(hashrate_label, LV_ALIGN_TOP_MID, 0, 15);

    // Unit label
    unit_label = lv_label_create(night_screen);
    lv_label_set_text(unit_label, "GH/s");
    lv_obj_set_style_text_color(unit_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_30, 0);
    lv_obj_align_to(unit_label, hashrate_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // hashrate chart without borders
    hashrate_chart = lv_chart_create(night_screen);
    lv_obj_set_size(hashrate_chart, SCREEN_WIDTH - 70, SCREEN_HEIGHT - 140);
    lv_obj_align(hashrate_chart, LV_ALIGN_BOTTOM_RIGHT, 0, -48);
    lv_obj_set_style_bg_color(hashrate_chart, COLOR_NIGHT_BG, 0);
    lv_obj_set_style_bg_opa(hashrate_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hashrate_chart, 0, 0);
    lv_obj_set_style_radius(hashrate_chart, 0, 0);
    lv_obj_set_style_pad_all(hashrate_chart, 0, 0);
    lv_obj_set_style_pad_top(hashrate_chart, 8, 0);
    lv_obj_set_style_pad_bottom(hashrate_chart, 40, 0);
    lv_obj_clear_flag(hashrate_chart, LV_OBJ_FLAG_SCROLLABLE);

    // Configure chart with right-to-left scrolling
    lv_chart_set_type(hashrate_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(hashrate_chart, NIGHT_SCREEN_HASHRATE_POINTS);
    lv_chart_set_range(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // Will adjust based on data
    lv_chart_set_update_mode(hashrate_chart, LV_CHART_UPDATE_MODE_SHIFT);

    // Style appearance with area fill
    lv_obj_set_style_text_color(hashrate_chart, COLOR_TEXT_PRIMARY, LV_PART_TICKS);
    lv_obj_set_style_text_opa(hashrate_chart, LV_OPA_COVER, LV_PART_TICKS);
    lv_obj_set_style_line_color(hashrate_chart, COLOR_TEXT_PRIMARY, LV_PART_TICKS);
    lv_obj_set_style_line_width(hashrate_chart, 1, LV_PART_TICKS);
    lv_obj_set_style_text_font(hashrate_chart, &lv_font_montserrat_16, LV_PART_TICKS);
    lv_obj_set_style_text_color(hashrate_chart, COLOR_NIGHT_BG, LV_PART_MAIN);
    lv_obj_set_style_line_color(hashrate_chart, COLOR_NIGHT_ACCENT, LV_PART_ITEMS);
    lv_obj_set_style_line_width(hashrate_chart, 4, LV_PART_ITEMS);
    lv_obj_set_style_size(hashrate_chart, 0, LV_PART_INDICATOR);

    // Add area fill with gradient effect
    lv_obj_set_style_bg_color(hashrate_chart, COLOR_NIGHT_ACCENT, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(hashrate_chart, LV_OPA_30, LV_PART_ITEMS);
    lv_obj_set_style_bg_grad_color(hashrate_chart, COLOR_NIGHT_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_grad_dir(hashrate_chart, LV_GRAD_DIR_VER, LV_PART_ITEMS);

    // Add subtle glow effect
    lv_obj_set_style_shadow_color(hashrate_chart, COLOR_NIGHT_ACCENT, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(hashrate_chart, 8, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(hashrate_chart, LV_OPA_20, LV_PART_ITEMS);
    lv_obj_set_style_shadow_spread(hashrate_chart, 2, LV_PART_ITEMS);

    // Show Y axis ticks/labels, keep X axis hidden
    lv_chart_set_axis_tick(hashrate_chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 0, 0, false, 0);
    lv_chart_set_axis_tick(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 6, 1, true, 70);

    lv_chart_set_div_line_count(hashrate_chart, 0, 0);

    hashrate_series = lv_chart_add_series(hashrate_chart, COLOR_NIGHT_ACCENT, LV_CHART_AXIS_PRIMARY_Y);

    // Initialize chart with invalid values to avoid ugly zero line
    for (int i = 0; i < NIGHT_SCREEN_HASHRATE_POINTS; i++)
    {
        lv_chart_set_next_value(hashrate_chart, hashrate_series, LV_CHART_POINT_NONE);
        if (history_count == 0)
        {
            hashrate_history[i] = -1.0f; // Use -1 to indicate no data
        }
    }

    apply_cached_hashrate();
    create_bottom_nav();
}

void night_screen_destroy(void)
{
    if (night_screen)
    {
        lv_obj_del(night_screen);
        night_screen = NULL;
        hashrate_label = NULL;
        unit_label = NULL;
        hashrate_chart = NULL;
        hashrate_series = NULL;
    }
}

lv_obj_t *night_get_screen(void)
{
    return night_screen;
}

void night_update_hashrate(const char *hashrate)
{
    if (!hashrate)
        return;

    strncpy(last_hashrate_text, hashrate, sizeof(last_hashrate_text) - 1);
    last_hashrate_text[sizeof(last_hashrate_text) - 1] = '\0';

    // Convert string to float for chart
    float hashrate_value = atof(hashrate);

    // Mark that we've received first real data
    if (!first_data_received && hashrate_value > 0)
    {
        first_data_received = true;
        if (hashrate_chart && hashrate_series)
        {
            // Reset all chart points to start fresh with real data
            for (int i = 0; i < NIGHT_SCREEN_HASHRATE_POINTS; i++)
            {
                lv_chart_set_value_by_id(hashrate_chart, hashrate_series, i, LV_CHART_POINT_NONE);
            }
        }
    }

    // Store in history
    hashrate_history[history_index] = hashrate_value;
    history_index = (history_index + 1) % NIGHT_SCREEN_HASHRATE_POINTS;
    if (history_count < NIGHT_SCREEN_HASHRATE_POINTS)
    {
        history_count++;
    }

    if (!hashrate_label || !hashrate_chart || !hashrate_series)
    {
        return;
    }

    // Update the large hashrate display
    lv_label_set_text(hashrate_label, hashrate);
    if (unit_label)
    {
        lv_obj_align_to(unit_label, hashrate_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }

    // Add to chart - data flows from right to left with SHIFT mode
    lv_chart_set_next_value(hashrate_chart, hashrate_series, (lv_coord_t)hashrate_value);

    // Dynamically adjust Y-axis range based on actual data
    float max_value = 0.0f;
    float min_value = hashrate_value;
    for (int i = 0; i < history_count; i++)
    {
        if (hashrate_history[i] >= 0)
        { // Only consider valid data points
            if (hashrate_history[i] > max_value)
            {
                max_value = hashrate_history[i];
            }
            if (hashrate_history[i] < min_value)
            {
                min_value = hashrate_history[i];
            }
        }
    }

    // Set range with padding for better visual appearance
    if (max_value > 0)
    {
        float range_padding = (max_value - min_value) * 0.1f; // 10% padding
        float y_min = (min_value > range_padding) ? (min_value - range_padding) : 0;
        float y_max = max_value + range_padding + 5; // Extra padding at top

        lv_chart_set_range(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y,
                           (lv_coord_t)y_min, (lv_coord_t)y_max);
    }
}

static void apply_cached_hashrate(void)
{
    if (hashrate_label)
    {
        lv_label_set_text(hashrate_label, last_hashrate_text);
    }
    if (unit_label && hashrate_label)
    {
        lv_obj_align_to(unit_label, hashrate_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (!hashrate_chart || !hashrate_series)
    {
        return;
    }

    // Rebuild chart from history buffer with newest on the right
    int start = (history_index - history_count + NIGHT_SCREEN_HASHRATE_POINTS) % NIGHT_SCREEN_HASHRATE_POINTS;
    int empty_prefix = NIGHT_SCREEN_HASHRATE_POINTS - history_count;
    float max_value = 0.0f;
    float min_value = 0.0f;
    bool have_values = false;

    for (int i = 0; i < NIGHT_SCREEN_HASHRATE_POINTS; i++)
    {
        if (i < empty_prefix)
        {
            lv_chart_set_value_by_id(hashrate_chart, hashrate_series, i, LV_CHART_POINT_NONE);
            continue;
        }

        int hist_idx = (start + (i - empty_prefix)) % NIGHT_SCREEN_HASHRATE_POINTS;
        float value = hashrate_history[hist_idx];
        if (value < 0.0f)
        {
            lv_chart_set_value_by_id(hashrate_chart, hashrate_series, i, LV_CHART_POINT_NONE);
            continue;
        }

        lv_chart_set_value_by_id(hashrate_chart, hashrate_series, i, (lv_coord_t)value);
        if (!have_values)
        {
            min_value = value;
            max_value = value;
            have_values = true;
        }
        else
        {
            if (value > max_value)
            {
                max_value = value;
            }
            if (value < min_value)
            {
                min_value = value;
            }
        }
    }

    if (have_values)
    {
        float range_padding = (max_value - min_value) * 0.1f;
        float y_min = (min_value > range_padding) ? (min_value - range_padding) : 0;
        float y_max = max_value + range_padding + 5;
        lv_chart_set_range(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y,
                           (lv_coord_t)y_min, (lv_coord_t)y_max);
    }
}

static void create_bottom_nav(void)
{
    lv_obj_t *bottom_nav = lv_obj_create(night_screen);
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

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, night_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_STOP, night_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, night_mempool_clicked, false);
    create_bottom_nav_btn(bottom_nav, FA_CLOCK, night_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", night_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, night_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, night_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, NULL, true);
}

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 56, 46);
    lv_obj_set_style_bg_color(btn, active ? COLOR_NIGHT_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_NIGHT_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_TEXT_ON_ACCENT : COLOR_NIGHT_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

void night_home_clicked(lv_event_t *e)
{
    home_screen_create();
    lv_scr_load(home_get_screen());
    night_screen_destroy();
}

void night_wifi_clicked(lv_event_t *e)
{
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    night_screen_destroy();
}

void night_block_clicked(lv_event_t *e)
{
    block_screen_create();
    lv_scr_load(block_get_screen());
    night_screen_destroy();
}

void night_mempool_clicked(lv_event_t *e)
{
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    night_screen_destroy();
}

void night_clock_clicked(lv_event_t *e)
{
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    night_screen_destroy();
}

void night_price_clicked(lv_event_t *e)
{
    price_screen_create();
    lv_scr_load(price_get_screen());
    night_screen_destroy();
}

void night_settings_clicked(lv_event_t *e)
{
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    night_screen_destroy();
}
