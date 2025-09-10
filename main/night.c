#include "night.h"
#include "home.h"
#include "settings.h"
#include "wifi.h"
#include "bap.h"
#include "custom_fonts.h"
#include "stdio.h"
#include "string.h"

#define COLOR_NIGHT_BG     lv_color_hex(0x1a0000)
#define COLOR_NIGHT_ACCENT lv_color_hex(0xff3333)
#define COLOR_NIGHT_TEXT   lv_color_hex(0xff9999)
#define COLOR_NIGHT_CARD   lv_color_hex(0x330000)

#define NIGHT_SCREEN_HASHRATE_POINTS 100

static lv_obj_t * night_screen = NULL;
static lv_obj_t * hashrate_label = NULL;
static lv_obj_t * hashrate_chart = NULL;
static lv_chart_series_t * hashrate_series = NULL;

static float hashrate_history[NIGHT_SCREEN_HASHRATE_POINTS];
static int history_index = 0;
static int history_count = 0;
static bool first_data_received = false;

static void create_bottom_nav(void);
static lv_obj_t* create_bottom_nav_btn(lv_obj_t* parent, const char* symbol, lv_event_cb_t event_cb, bool active);

void night_screen_create(void)
{
    if(night_screen != NULL) {
        return;
    }
    
    night_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(night_screen, COLOR_NIGHT_BG, 0);
    lv_obj_set_style_bg_opa(night_screen, LV_OPA_COVER, 0);
    
    // Large hashrate display at the top
    hashrate_label = lv_label_create(night_screen);
    lv_label_set_text(hashrate_label, "0.00");
    lv_obj_set_style_text_color(hashrate_label, COLOR_NIGHT_ACCENT, 0);
    lv_obj_set_style_text_font(hashrate_label, &lv_font_montserrat_48, 0);
    lv_obj_align(hashrate_label, LV_ALIGN_TOP_MID, 0, 30);
    
    // Unit label
    lv_obj_t * unit_label = lv_label_create(night_screen);
    lv_label_set_text(unit_label, "GH/s");
    lv_obj_set_style_text_color(unit_label, COLOR_NIGHT_TEXT, 0);
    lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(unit_label, hashrate_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 10);
    
    //hashrate chart without borders
    hashrate_chart = lv_chart_create(night_screen);
    lv_obj_set_size(hashrate_chart, SCREEN_WIDTH, SCREEN_HEIGHT - 120);
    lv_obj_align(hashrate_chart, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_set_style_bg_color(hashrate_chart, COLOR_NIGHT_BG, 0);
    lv_obj_set_style_bg_opa(hashrate_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hashrate_chart, 0, 0);
    lv_obj_set_style_radius(hashrate_chart, 0, 0);
    lv_obj_set_style_pad_all(hashrate_chart, 0, 0);
    
    // Configure chart with right-to-left scrolling
    lv_chart_set_type(hashrate_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(hashrate_chart, NIGHT_SCREEN_HASHRATE_POINTS);
    lv_chart_set_range(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // Will adjust based on data
    lv_chart_set_update_mode(hashrate_chart, LV_CHART_UPDATE_MODE_SHIFT);
    
    // Style appearance with area fill
    lv_obj_set_style_text_color(hashrate_chart, COLOR_NIGHT_TEXT, LV_PART_TICKS);
    lv_obj_set_style_text_color(hashrate_chart, COLOR_NIGHT_BG, LV_PART_MAIN);
    lv_obj_set_style_line_color(hashrate_chart, COLOR_NIGHT_ACCENT, LV_PART_ITEMS);
    lv_obj_set_style_line_width(hashrate_chart, 4, LV_PART_ITEMS);
    lv_obj_set_style_size(hashrate_chart, 0, LV_PART_INDICATOR);
    
    // Add area fill with gradient effect
    lv_obj_set_style_bg_color(hashrate_chart, lv_color_hex(0x4d1a1a), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(hashrate_chart, LV_OPA_30, LV_PART_ITEMS);
    lv_obj_set_style_bg_grad_color(hashrate_chart, COLOR_NIGHT_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_grad_dir(hashrate_chart, LV_GRAD_DIR_VER, LV_PART_ITEMS);
    
    // Add subtle glow effect
    lv_obj_set_style_shadow_color(hashrate_chart, COLOR_NIGHT_ACCENT, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(hashrate_chart, 8, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(hashrate_chart, LV_OPA_20, LV_PART_ITEMS);
    lv_obj_set_style_shadow_spread(hashrate_chart, 2, LV_PART_ITEMS);
    
    // Hide axes and grid lines for cleaner look
    lv_chart_set_axis_tick(hashrate_chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 0, 0, false, 0);
    lv_chart_set_axis_tick(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 0, 0, false, 0);
    lv_chart_set_div_line_count(hashrate_chart, 0, 0);
    
    hashrate_series = lv_chart_add_series(hashrate_chart, COLOR_NIGHT_ACCENT, LV_CHART_AXIS_PRIMARY_Y);
    
    // Initialize chart with invalid values to avoid ugly zero line
    for(int i = 0; i < NIGHT_SCREEN_HASHRATE_POINTS; i++) {
        lv_chart_set_next_value(hashrate_chart, hashrate_series, LV_CHART_POINT_NONE);
        hashrate_history[i] = -1.0f; // Use -1 to indicate no data
    }
    
    create_bottom_nav();
}

void night_screen_destroy(void)
{
    if(night_screen) {
        lv_obj_del(night_screen);
        night_screen = NULL;
        hashrate_label = NULL;
        hashrate_chart = NULL;
        hashrate_series = NULL;
        history_index = 0;
        history_count = 0;
        first_data_received = false;
    }
}

lv_obj_t* night_get_screen(void)
{
    return night_screen;
}

void night_update_hashrate(const char* hashrate)
{
    if(!hashrate_label || !hashrate_chart || !hashrate_series) return;
    
    // Update the large hashrate display
    lv_label_set_text(hashrate_label, hashrate);
    
    // Convert string to float for chart
    float hashrate_value = atof(hashrate);
    
    // Mark that we've received first real data
    if(!first_data_received && hashrate_value > 0) {
        first_data_received = true;
        // Reset all chart points to start fresh with real data
        for(int i = 0; i < NIGHT_SCREEN_HASHRATE_POINTS; i++) {
            lv_chart_set_value_by_id(hashrate_chart, hashrate_series, i, LV_CHART_POINT_NONE);
        }
    }
    
    // Store in history
    hashrate_history[history_index] = hashrate_value;
    history_index = (history_index + 1) % NIGHT_SCREEN_HASHRATE_POINTS;
    if(history_count < NIGHT_SCREEN_HASHRATE_POINTS) {
        history_count++;
    }
    
    // Add to chart - data flows from right to left with SHIFT mode
    lv_chart_set_next_value(hashrate_chart, hashrate_series, (lv_coord_t)hashrate_value);
    
    // Dynamically adjust Y-axis range based on actual data
    float max_value = 0.0f;
    float min_value = hashrate_value;
    for(int i = 0; i < history_count; i++) {
        if(hashrate_history[i] >= 0) { // Only consider valid data points
            if(hashrate_history[i] > max_value) {
                max_value = hashrate_history[i];
            }
            if(hashrate_history[i] < min_value) {
                min_value = hashrate_history[i];
            }
        }
    }
    
    // Set range with padding for better visual appearance
    if(max_value > 0) {
        float range_padding = (max_value - min_value) * 0.1f; // 10% padding
        float y_min = (min_value > range_padding) ? (min_value - range_padding) : 0;
        float y_max = max_value + range_padding + 5; // Extra padding at top
        
        lv_chart_set_range(hashrate_chart, LV_CHART_AXIS_PRIMARY_Y, 
                          (lv_coord_t)y_min, (lv_coord_t)y_max);
    }
}

static void create_bottom_nav(void)
{
    lv_obj_t * bottom_nav = lv_obj_create(night_screen);
    lv_obj_set_size(bottom_nav, SCREEN_WIDTH, 70);
    lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_nav, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_nav, 0, 0);
    lv_obj_set_style_radius(bottom_nav, 0, 0);
    lv_obj_set_style_pad_all(bottom_nav, 10, 0);
    lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, night_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, night_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, night_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_CLOSE, NULL, true);
}

static lv_obj_t* create_bottom_nav_btn(lv_obj_t* parent, const char* symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 60, 50);
    lv_obj_set_style_bg_color(btn, active ? COLOR_NIGHT_ACCENT : lv_color_hex(0x4A5F6F), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_NIGHT_BG : COLOR_NIGHT_TEXT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
    
    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

void night_home_clicked(lv_event_t * e)
{
    home_screen_create();
    lv_scr_load(home_get_screen());
    night_screen_destroy();
}

void night_wifi_clicked(lv_event_t * e)
{
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    night_screen_destroy();
}

void night_settings_clicked(lv_event_t * e)
{
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    night_screen_destroy();
}