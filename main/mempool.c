#include "mempool.h"
#include "home.h"
#include "block.h"
#include "clock.h"
#include "price.h"
#include "wifi.h"
#include "settings.h"
#include "night.h"
#include "lvgl_port.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MEMPOOL_HTTP_BUF_SIZE 65536
#define MEMPOOL_MAX_BLOCKS 8
#define MEMPOOL_FETCH_INTERVAL_MS 60000

#define CARD_W 210
#define CARD_H 210

static const char *MEMPOOL_API_URL = "https://mempool.space/api/v1/blocks";

typedef struct
{
    long long height;
    long long timestamp;
    int tx_count;
    double median_fee;
    double fee_min;
    double fee_max;
    long long total_fees_sat;
    char pool_name[32];
    int minutes_ago;
} mempool_block_t;

static lv_obj_t *mempool_screen = NULL;
static lv_obj_t *mempool_status_label = NULL;
static lv_obj_t *mempool_row = NULL;
static TaskHandle_t mempool_task_handle = NULL;
static bool mempool_netif_ready = false;
static bool mempool_log_tuned = false;
static bool mempool_sntp_started = false;

static mempool_block_t mempool_blocks[MEMPOOL_MAX_BLOCKS];
static int mempool_block_count = 0;

static char mempool_http_buf[MEMPOOL_HTTP_BUF_SIZE];
static int mempool_http_len = 0;

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active);
static void mempool_task(void *arg);
static bool mempool_fetch_once(void);
static bool mempool_ensure_netif(void);
static bool mempool_wifi_connected(void);
static bool mempool_ip_ready(void);
static void mempool_start_sntp(void);
static bool mempool_time_ready(void);
static void mempool_set_status(const char *status);
static void mempool_rebuild_cards(void);

static bool json_get_ll(const char *obj, const char *key, long long *out);
static bool json_get_double(const char *obj, const char *key, double *out);
static void format_fee_value(double v, char *buf, size_t buf_size);
static void format_btc_from_sats(long long sats, char *buf, size_t buf_size);
static int split_top_level_objects(const char *json, int len, int starts[], int ends[], int max_items);
static bool parse_fee_range(const char *obj, double *out_min, double *out_max);
static void parse_pool_name(const char *obj, char *out, size_t out_size);
static int compute_minutes_ago(long long ts);

static esp_err_t mempool_http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0)
    {
        int copy_len = evt->data_len;
        if (mempool_http_len + copy_len >= MEMPOOL_HTTP_BUF_SIZE)
        {
            copy_len = MEMPOOL_HTTP_BUF_SIZE - mempool_http_len - 1;
        }
        if (copy_len > 0)
        {
            memcpy(mempool_http_buf + mempool_http_len, evt->data, copy_len);
            mempool_http_len += copy_len;
            mempool_http_buf[mempool_http_len] = '\0';
        }
    }
    return ESP_OK;
}

void mempool_screen_create(void)
{
    if (mempool_screen != NULL)
    {
        return;
    }

    if (!mempool_log_tuned)
    {
        // Timeouts are expected on unreliable networks; keep logs readable.
        esp_log_level_set("esp-tls", ESP_LOG_WARN);
        esp_log_level_set("HTTP_CLIENT", ESP_LOG_WARN);
        esp_log_level_set("transport_base", ESP_LOG_WARN);
        mempool_log_tuned = true;
    }

    mempool_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(mempool_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(mempool_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(mempool_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(mempool_screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(mempool_screen);
    lv_label_set_text(title, "LATEST BLOCKS");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    mempool_status_label = lv_label_create(mempool_screen);
    lv_label_set_text(mempool_status_label, "LOADING...");
    lv_obj_set_style_text_color(mempool_status_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_opa(mempool_status_label, (lv_opa_t)192, 0);
    lv_obj_set_style_text_font(mempool_status_label, &lv_font_montserrat_16, 0);
    lv_obj_align(mempool_status_label, LV_ALIGN_TOP_MID, 0, 48);

    mempool_row = lv_obj_create(mempool_screen);
    const int row_y = 76;
    const int nav_h = 64;
    const int row_bottom_gap = 16;
    int row_h = SCREEN_HEIGHT - row_y - nav_h - row_bottom_gap;
    if (row_h < 300)
    {
        row_h = 300;
    }
    lv_obj_set_size(mempool_row, SCREEN_WIDTH, row_h);
    lv_obj_align(mempool_row, LV_ALIGN_TOP_MID, 0, row_y);
    lv_obj_set_style_bg_opa(mempool_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mempool_row, 0, 0);
    lv_obj_set_style_pad_left(mempool_row, 18, 0);
    lv_obj_set_style_pad_right(mempool_row, 18, 0);
    lv_obj_set_style_pad_top(mempool_row, 0, 0);
    lv_obj_set_style_pad_bottom(mempool_row, 0, 0);
    lv_obj_set_style_pad_column(mempool_row, 24, 0);
    lv_obj_set_scroll_dir(mempool_row, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(mempool_row, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(mempool_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mempool_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *bottom_nav = lv_obj_create(mempool_screen);
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

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, mempool_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_STOP, mempool_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, NULL, true);
    create_bottom_nav_btn(bottom_nav, FA_CLOCK, mempool_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", mempool_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, mempool_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, mempool_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, mempool_night_clicked, false);

    mempool_rebuild_cards();

    if (mempool_task_handle == NULL)
    {
        xTaskCreate(mempool_task, "mempool_task", 6144, NULL, 5, &mempool_task_handle);
    }
}

void mempool_screen_destroy(void)
{
    if (mempool_screen)
    {
        lv_obj_del(mempool_screen);
        mempool_screen = NULL;
        mempool_status_label = NULL;
        mempool_row = NULL;
    }
}

lv_obj_t *mempool_get_screen(void)
{
    return mempool_screen;
}

static void mempool_task(void *arg)
{
    (void)arg;
    while (1)
    {
        if (!mempool_wifi_connected())
        {
            if (lvgl_port_lock(50))
            {
                mempool_set_status("WAITING FOR WIFI");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!mempool_ensure_netif())
        {
            if (lvgl_port_lock(50))
            {
                mempool_set_status("NETIF ERROR");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!mempool_ip_ready())
        {
            if (lvgl_port_lock(50))
            {
                mempool_set_status("WAITING FOR IP");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        mempool_start_sntp();
        if (!mempool_time_ready())
        {
            if (lvgl_port_lock(50))
            {
                mempool_set_status("SYNCING TIME");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (lvgl_port_lock(50))
        {
            mempool_set_status("LOADING...");
            lvgl_port_unlock();
        }

        bool updated = mempool_fetch_once();
        if (lvgl_port_lock(50))
        {
            if (updated)
            {
                mempool_set_status("LIVE (from mempool.space)");
                mempool_rebuild_cards();
                lvgl_port_unlock();
                vTaskDelay(pdMS_TO_TICKS(MEMPOOL_FETCH_INTERVAL_MS));
                continue;
            }

            mempool_set_status("RETRYING...");
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static bool mempool_fetch_once(void)
{
    esp_http_client_config_t config = {
        .url = MEMPOOL_API_URL,
        .event_handler = mempool_http_event_handler,
        .timeout_ms = 12000,
    };

#if defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) && CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY
    // Optional insecure mode to reduce TLS overhead on constrained links.
    config.skip_cert_common_name_check = true;
#else
    config.crt_bundle_attach = esp_crt_bundle_attach;
#endif

    mempool_http_len = 0;
    mempool_http_buf[0] = '\0';

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status < 200 || status >= 300 || mempool_http_len == 0)
    {
        return false;
    }

    int starts[MEMPOOL_MAX_BLOCKS];
    int ends[MEMPOOL_MAX_BLOCKS];
    int obj_count = split_top_level_objects(mempool_http_buf, mempool_http_len, starts, ends, MEMPOOL_MAX_BLOCKS);
    if (obj_count <= 0)
    {
        return false;
    }

    mempool_block_count = 0;

    for (int i = 0; i < obj_count && i < MEMPOOL_MAX_BLOCKS; i++)
    {
        int len = ends[i] - starts[i] + 1;
        if (len <= 0 || len > MEMPOOL_HTTP_BUF_SIZE - 1)
        {
            continue;
        }

        char *obj = malloc((size_t)len + 1);
        if (!obj)
        {
            continue;
        }
        memcpy(obj, mempool_http_buf + starts[i], (size_t)len);
        obj[len] = '\0';

        mempool_block_t block = {0};
        long long ts = 0;
        long long height = 0;
        long long txc = 0;
        double median = 0.0;
        double fee_min = 0.0;
        double fee_max = 0.0;
        long long total_fees = 0;

        if (!json_get_ll(obj, "\"height\":", &height))
        {
            free(obj);
            continue;
        }
        json_get_ll(obj, "\"timestamp\":", &ts);
        json_get_ll(obj, "\"tx_count\":", &txc);
        json_get_double(obj, "\"medianFee\":", &median);
        json_get_ll(obj, "\"totalFees\":", &total_fees);
        parse_fee_range(obj, &fee_min, &fee_max);
        parse_pool_name(obj, block.pool_name, sizeof(block.pool_name));

        block.height = height;
        block.timestamp = ts;
        block.tx_count = (int)txc;
        block.median_fee = median;
        block.fee_min = fee_min;
        block.fee_max = fee_max;
        block.total_fees_sat = total_fees;
        block.minutes_ago = compute_minutes_ago(ts);

        mempool_blocks[mempool_block_count++] = block;
        free(obj);
    }

    return mempool_block_count > 0;
}

static bool mempool_ensure_netif(void)
{
    if (mempool_netif_ready)
    {
        return true;
    }

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return false;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return false;
    }

    mempool_netif_ready = true;
    return true;
}

static bool mempool_wifi_connected(void)
{
    return wifi_is_connected();
}

static bool mempool_ip_ready(void)
{
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta)
    {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(sta, &ip_info) != ESP_OK)
    {
        return false;
    }

    // 0.0.0.0 means DHCP hasn't completed yet.
    return ip_info.ip.addr != 0;
}

static void mempool_start_sntp(void)
{
    if (mempool_sntp_started || sntp_enabled())
    {
        mempool_sntp_started = true;
        return;
    }

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    mempool_sntp_started = true;
}

static bool mempool_time_ready(void)
{
    time_t now = time(NULL);
    struct tm time_info;
    localtime_r(&now, &time_info);
    // Pre-2023 typically means SNTP hasn't synced yet.
    return time_info.tm_year >= (2023 - 1900);
}

static void mempool_set_status(const char *status)
{
    if (!status || !mempool_status_label)
    {
        return;
    }

    lv_label_set_text(mempool_status_label, status);
}

static void mempool_rebuild_cards(void)
{
    if (!mempool_row)
    {
        return;
    }

    lv_obj_clean(mempool_row);

    if (mempool_block_count <= 0)
    {
        return;
    }

    const lv_color_t color_height = lv_color_hex(0x00E5FF);
    const lv_color_t color_card = lv_color_hex(0x0B1E3A);
    const lv_color_t color_mid = lv_color_hex(0x1E5BFF);
    const lv_color_t color_bottom = lv_color_hex(0x7C3BFF);
    const lv_color_t color_fee = lv_color_hex(0xFFE600);
    const int row_h = lv_obj_get_height(mempool_row);
    const int wrap_top_offset = 38;
    const int wrap_pool_h = 24;
    const int wrap_bottom_pad = 8;
    const int wrap_extra = wrap_top_offset + wrap_pool_h + wrap_bottom_pad;
    int card_h = row_h - wrap_extra;
    if (card_h > CARD_H)
    {
        card_h = CARD_H;
    }
    if (card_h < 200)
    {
        card_h = 200;
    }

    for (int i = 0; i < mempool_block_count; i++)
    {
        mempool_block_t *b = &mempool_blocks[i];

        lv_obj_t *card_wrap = lv_obj_create(mempool_row);
        lv_obj_set_size(card_wrap, CARD_W, card_h + wrap_extra);
        lv_obj_set_style_bg_opa(card_wrap, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(card_wrap, 0, 0);
        lv_obj_set_style_pad_all(card_wrap, 0, 0);
        lv_obj_clear_flag(card_wrap, LV_OBJ_FLAG_SCROLLABLE);

        char height_txt[16];
        lv_snprintf(height_txt, sizeof(height_txt), "%lld", b->height);
        lv_obj_t *height_label = lv_label_create(card_wrap);
        lv_label_set_text(height_label, height_txt);
        lv_obj_set_style_text_color(height_label, color_height, 0);
        lv_obj_set_style_text_font(height_label, &lv_font_montserrat_24, 0);
        lv_obj_align(height_label, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t *card = lv_obj_create(card_wrap);
        lv_obj_set_size(card, CARD_W, card_h);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, wrap_top_offset);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_set_style_bg_color(card, color_card, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_clip_corner(card, true, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *mid = lv_obj_create(card);
        lv_obj_set_size(mid, CARD_W, card_h);
        lv_obj_align(mid, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_radius(mid, 8, 0);
        lv_obj_set_style_bg_color(mid, color_mid, 0);
        lv_obj_set_style_bg_grad_color(mid, color_bottom, 0);
        lv_obj_set_style_bg_grad_dir(mid, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_opa(mid, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(mid, 0, 0);
        lv_obj_set_style_pad_all(mid, 0, 0);
        lv_obj_clear_flag(mid, LV_OBJ_FLAG_SCROLLABLE);

        const int bottom_bar_h = 52;
        int content_h = card_h - bottom_bar_h;
        if (content_h < 120)
        {
            content_h = 120;
        }
        const int median_y = 10;
        const int range_y = median_y + 28;
        const int btc_y = range_y + 34;
        int tx_y = btc_y + 44;
        if (tx_y > content_h - 28)
        {
            tx_y = content_h - 28;
        }

        char median_txt[32];
        int median_fee_i = (int)(b->median_fee + 0.5);
        lv_snprintf(median_txt, sizeof(median_txt), "~%d sat/vB", median_fee_i);
        lv_obj_t *median_label = lv_label_create(card);
        lv_label_set_text(median_label, median_txt);
        lv_obj_set_style_text_color(median_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(median_label, &lv_font_montserrat_20, 0);
        lv_obj_align(median_label, LV_ALIGN_TOP_MID, 0, median_y);

        char fee_min_txt[16];
        char fee_max_txt[16];
        format_fee_value(b->fee_min, fee_min_txt, sizeof(fee_min_txt));
        format_fee_value(b->fee_max, fee_max_txt, sizeof(fee_max_txt));
        char range_txt[48];
        lv_snprintf(range_txt, sizeof(range_txt), "%s - %s sat/vB", fee_min_txt, fee_max_txt);
        lv_obj_t *range_label = lv_label_create(card);
        lv_label_set_text(range_label, range_txt);
        lv_obj_set_style_text_color(range_label, color_fee, 0);
        lv_obj_set_style_text_font(range_label, &lv_font_montserrat_48, 0);
        lv_obj_align(range_label, LV_ALIGN_TOP_MID, 0, range_y);

        char btc_txt[32];
        format_btc_from_sats(b->total_fees_sat, btc_txt, sizeof(btc_txt));
        lv_obj_t *btc_label = lv_label_create(card);
        lv_label_set_text(btc_label, btc_txt);
        lv_obj_set_style_text_color(btc_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(btc_label, &lv_font_montserrat_28, 0);
        lv_obj_align(btc_label, LV_ALIGN_TOP_MID, 0, btc_y);

        char tx_txt[40];
        lv_snprintf(tx_txt, sizeof(tx_txt), "%d transactions", b->tx_count);
        lv_obj_t *tx_label = lv_label_create(card);
        lv_label_set_text(tx_label, tx_txt);
        lv_obj_set_style_text_color(tx_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(tx_label, &lv_font_montserrat_48, 0);
        lv_obj_align(tx_label, LV_ALIGN_TOP_MID, 0, tx_y);

        lv_obj_t *bottom_bar = lv_obj_create(card);
        lv_obj_set_size(bottom_bar, CARD_W, bottom_bar_h);
        lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_radius(bottom_bar, 0, 0);
        lv_obj_set_style_bg_color(bottom_bar, color_bottom, 0);
        lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bottom_bar, 0, 0);
        lv_obj_set_style_pad_all(bottom_bar, 0, 0);
        lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

        char ago_txt[32];
        if (b->minutes_ago >= 0)
        {
            lv_snprintf(ago_txt, sizeof(ago_txt), "%d minutes ago", b->minutes_ago);
        }
        else
        {
            lv_snprintf(ago_txt, sizeof(ago_txt), "-- minutes ago");
        }
        lv_obj_t *ago_label = lv_label_create(bottom_bar);
        lv_label_set_text(ago_label, ago_txt);
        lv_obj_set_style_text_color(ago_label, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(ago_label, &lv_font_montserrat_20, 0);
        lv_obj_center(ago_label);

        if (b->pool_name[0] != '\0')
        {
            lv_obj_t *pool_label = lv_label_create(card_wrap);
            lv_label_set_text(pool_label, b->pool_name);
            lv_label_set_long_mode(pool_label, LV_LABEL_LONG_DOT);
            lv_obj_set_width(pool_label, CARD_W);
            lv_obj_set_style_text_color(pool_label, COLOR_TEXT_PRIMARY, 0);
            lv_obj_set_style_text_font(pool_label, &lv_font_montserrat_48, 0);
            lv_obj_set_style_text_align(pool_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(pool_label, LV_ALIGN_BOTTOM_MID, 0, -wrap_bottom_pad);
        }
    }
}

static bool json_get_ll(const char *obj, const char *key, long long *out)
{
    if (!obj || !key || !out)
    {
        return false;
    }

    const char *p = strstr(obj, key);
    if (!p)
    {
        return false;
    }
    p += strlen(key);
    *out = strtoll(p, NULL, 10);
    return true;
}

static bool json_get_double(const char *obj, const char *key, double *out)
{
    if (!obj || !key || !out)
    {
        return false;
    }

    const char *p = strstr(obj, key);
    if (!p)
    {
        return false;
    }
    p += strlen(key);
    *out = strtod(p, NULL);
    return true;
}

static void format_fee_value(double v, char *buf, size_t buf_size)
{
    int v_i = (int)(v + 0.5);
    lv_snprintf(buf, buf_size, "%d", v_i);
}

static void format_btc_from_sats(long long sats, char *buf, size_t buf_size)
{
    if (sats < 0)
    {
        sats = 0;
    }

    long long whole = sats / 100000000LL;
    long long frac = sats % 100000000LL;
    long long frac3 = (frac + 50000LL) / 100000LL; // round to 3 decimals
    if (frac3 >= 1000LL)
    {
        whole += 1;
        frac3 = 0;
    }

    lv_snprintf(buf, buf_size, "%lld.%03lld BTC", whole, frac3);
}

static int split_top_level_objects(const char *json, int len, int starts[], int ends[], int max_items)
{
    int count = 0;
    int depth = 0;
    bool in_str = false;
    bool esc = false;
    int start_idx = -1;

    for (int i = 0; i < len; i++)
    {
        char c = json[i];
        if (in_str)
        {
            if (esc)
            {
                esc = false;
            }
            else if (c == '\\')
            {
                esc = true;
            }
            else if (c == '"')
            {
                in_str = false;
            }
            continue;
        }

        if (c == '"')
        {
            in_str = true;
            continue;
        }

        if (c == '{')
        {
            if (depth == 0)
            {
                start_idx = i;
            }
            depth++;
        }
        else if (c == '}')
        {
            depth--;
            if (depth == 0 && start_idx >= 0)
            {
                if (count < max_items)
                {
                    starts[count] = start_idx;
                    ends[count] = i;
                    count++;
                }
                else
                {
                    break;
                }
                start_idx = -1;
            }
        }
    }

    return count;
}

static bool parse_fee_range(const char *obj, double *out_min, double *out_max)
{
    if (!obj || !out_min || !out_max)
    {
        return false;
    }

    const char *p = strstr(obj, "\"feeRange\":[");
    if (!p)
    {
        return false;
    }
    p += strlen("\"feeRange\":[");

    double min_v = 0.0;
    double max_v = 0.0;
    bool have = false;

    while (*p && *p != ']')
    {
        char *endptr = NULL;
        double v = strtod(p, &endptr);
        if (endptr == p)
        {
            p++;
            continue;
        }
        if (!have)
        {
            min_v = max_v = v;
            have = true;
        }
        else
        {
            if (v < min_v)
                min_v = v;
            if (v > max_v)
                max_v = v;
        }
        p = endptr;
        if (*p == ',')
        {
            p++;
        }
    }

    if (!have)
    {
        return false;
    }

    *out_min = min_v;
    *out_max = max_v;
    return true;
}

static void parse_pool_name(const char *obj, char *out, size_t out_size)
{
    if (!obj || !out || out_size == 0)
    {
        return;
    }

    out[0] = '\0';
    const char *pool = strstr(obj, "\"pool\"");
    if (!pool)
    {
        return;
    }
    const char *name = strstr(pool, "\"name\":\"");
    if (!name)
    {
        return;
    }
    name += strlen("\"name\":\"");
    const char *end = strchr(name, '"');
    if (!end)
    {
        return;
    }

    size_t len = (size_t)(end - name);
    if (len >= out_size)
    {
        len = out_size - 1;
    }
    memcpy(out, name, len);
    out[len] = '\0';
}

static int compute_minutes_ago(long long ts)
{
    time_t now = time(NULL);
    if (now < 946684800 || ts <= 0)
    {
        return -1;
    }

    long long diff = (long long)now - ts;
    if (diff < 0)
    {
        diff = 0;
    }
    return (int)(diff / 60);
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
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_center(label);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

void mempool_home_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    home_screen_create();
    lv_scr_load(home_get_screen());
    mempool_screen_destroy();
}

void mempool_block_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    block_screen_create();
    lv_scr_load(block_get_screen());
    mempool_screen_destroy();
}

void mempool_clock_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    mempool_screen_destroy();
}

void mempool_price_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    price_screen_create();
    lv_scr_load(price_get_screen());
    mempool_screen_destroy();
}

void mempool_wifi_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    mempool_screen_destroy();
}

void mempool_settings_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    mempool_screen_destroy();
}

void mempool_night_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    night_screen_create();
    lv_scr_load(night_get_screen());
    mempool_screen_destroy();
}
