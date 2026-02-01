#include "price.h"
#include "home.h"
#include "block.h"
#include "clock.h"
#include "mempool.h"
#include "wifi.h"
#include "settings.h"
#include "night.h"
#include "custom_fonts.h"
#include "lvgl_port.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#define PRICE_HTTP_BUF_SIZE 512
#define PRICE_FETCH_INTERVAL_MS 60000

static const char *PRICE_API_URL = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";
static const char *PRICE_API_FALLBACK_URL = "https://api.coinbase.com/v2/prices/BTC-USD/spot";

static lv_obj_t *price_screen = NULL;
static lv_obj_t *price_value_cont = NULL;
static lv_obj_t *price_prefix_label = NULL;
static lv_obj_t *price_value_label = NULL;
static lv_obj_t *price_suffix_label = NULL;
static lv_obj_t *price_title_label = NULL;
static lv_obj_t *price_status_label = NULL;
static TaskHandle_t price_task_handle = NULL;
static bool price_netif_ready = false;

static char current_price_text[32] = "--";
static char current_price_status[24] = "LOADING...";

static lv_obj_t *create_bottom_nav_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t event_cb, bool active);
static void apply_cached_price(void);
static void price_task(void *arg);
static bool price_fetch_once(void);
static bool price_fetch_from_url(const char *url);
static bool price_parse_coingecko(const char *json, double *out_price);
static bool price_parse_coinbase(const char *json, double *out_price);
static bool price_ensure_netif(void);
static bool price_wifi_connected(void);
static void format_price_with_commas(long long value, char *out, size_t out_size);
static void price_set_status(const char *status);

static char price_http_buf[PRICE_HTTP_BUF_SIZE];
static int price_http_len = 0;

static esp_err_t price_http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0)
    {
        int copy_len = evt->data_len;
        if (price_http_len + copy_len >= PRICE_HTTP_BUF_SIZE)
        {
            copy_len = PRICE_HTTP_BUF_SIZE - price_http_len - 1;
        }
        if (copy_len > 0)
        {
            memcpy(price_http_buf + price_http_len, evt->data, copy_len);
            price_http_len += copy_len;
            price_http_buf[price_http_len] = '\0';
        }
    }
    return ESP_OK;
}

void price_screen_create(void)
{
    if (price_screen != NULL)
    {
        return;
    }

    price_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(price_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(price_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(price_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(price_screen, LV_SCROLLBAR_MODE_OFF);

    price_title_label = lv_label_create(price_screen);
    lv_label_set_text(price_title_label, "BTC PRICE (USD)");
    lv_obj_set_style_text_color(price_title_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(price_title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(price_title_label, LV_ALIGN_TOP_MID, 0, 30);

    price_status_label = lv_label_create(price_screen);
    lv_label_set_text(price_status_label, current_price_status);
    lv_obj_set_style_text_color(price_status_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_opa(price_status_label, (lv_opa_t)192, 0);
    lv_obj_set_style_text_font(price_status_label, &lv_font_montserrat_16, 0);
    lv_obj_align(price_status_label, LV_ALIGN_TOP_MID, 0, 58);

    price_value_cont = lv_obj_create(price_screen);
    lv_obj_set_size(price_value_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(price_value_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(price_value_cont, 0, 0);
    lv_obj_set_style_pad_all(price_value_cont, 0, 0);
    lv_obj_set_style_pad_column(price_value_cont, 10, 0);
    lv_obj_set_flex_flow(price_value_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(price_value_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(price_value_cont, LV_ALIGN_CENTER, 0, -10);

    price_prefix_label = lv_label_create(price_value_cont);
    lv_label_set_text(price_prefix_label, "$");
    lv_obj_set_style_text_color(price_prefix_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_opa(price_prefix_label, (lv_opa_t)192, 0);
    lv_obj_set_style_text_font(price_prefix_label, &montserrat_140, 0);

    price_value_label = lv_label_create(price_value_cont);
    lv_label_set_text(price_value_label, current_price_text);
    lv_obj_set_style_text_color(price_value_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(price_value_label, &montserrat_140, 0);
    lv_obj_set_style_text_letter_space(price_value_label, 2, 0);

    price_suffix_label = lv_label_create(price_value_cont);
    lv_label_set_text(price_suffix_label, "");
    lv_obj_set_style_text_color(price_suffix_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_opa(price_suffix_label, (lv_opa_t)192, 0);
    lv_obj_set_style_text_font(price_suffix_label, &lv_font_montserrat_48, 0);

    lv_obj_t *bottom_nav = lv_obj_create(price_screen);
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

    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, price_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_STOP, price_block_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_LIST, price_mempool_clicked, false);
    create_bottom_nav_btn_img(bottom_nav, &clock_solid_full, price_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", NULL, true);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, price_wifi_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, price_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, price_night_clicked, false);

    apply_cached_price();

    if (price_task_handle == NULL)
    {
        xTaskCreate(price_task, "price_fetch_task", 4096, NULL, 5, &price_task_handle);
    }
}

void price_screen_destroy(void)
{
    if (price_screen)
    {
        lv_obj_del(price_screen);
        price_screen = NULL;
        price_value_cont = NULL;
        price_prefix_label = NULL;
        price_value_label = NULL;
        price_suffix_label = NULL;
        price_title_label = NULL;
        price_status_label = NULL;
    }
}

lv_obj_t *price_get_screen(void)
{
    return price_screen;
}

static void apply_cached_price(void)
{
    if (price_value_label)
    {
        lv_label_set_text(price_value_label, current_price_text);
    }
    if (price_status_label)
    {
        lv_label_set_text(price_status_label, current_price_status);
    }
}

static bool price_fetch_once(void)
{
    if (!price_ensure_netif() || !price_wifi_connected())
    {
        return false;
    }

    if (price_fetch_from_url(PRICE_API_URL))
    {
        return true;
    }

    return price_fetch_from_url(PRICE_API_FALLBACK_URL);
}

static bool price_ensure_netif(void)
{
    if (price_netif_ready)
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

    price_netif_ready = true;
    return true;
}

static bool price_wifi_connected(void)
{
    return wifi_is_connected();
}

static bool price_fetch_from_url(const char *url)
{
    if (!url)
    {
        return false;
    }

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = price_http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 5000,
    };

    price_http_len = 0;
    price_http_buf[0] = '\0';

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status < 200 || status >= 300 || price_http_len == 0)
    {
        return false;
    }

    double price = 0.0;
    bool parsed = false;
    if (strstr(url, "coingecko"))
    {
        parsed = price_parse_coingecko(price_http_buf, &price);
    }
    else
    {
        parsed = price_parse_coinbase(price_http_buf, &price);
    }

    if (!parsed || price <= 0.0)
    {
        return false;
    }

    long long rounded_price = (long long)(price + 0.5);
    format_price_with_commas(rounded_price, current_price_text, sizeof(current_price_text));
    return true;
}

static bool price_parse_coingecko(const char *json, double *out_price)
{
    if (!json || !out_price)
    {
        return false;
    }

    const char *usd_ptr = strstr(json, "\"usd\":");
    if (!usd_ptr)
    {
        return false;
    }

    usd_ptr += 6;
    double price = strtod(usd_ptr, NULL);
    if (price <= 0.0)
    {
        return false;
    }

    *out_price = price;
    return true;
}

static bool price_parse_coinbase(const char *json, double *out_price)
{
    if (!json || !out_price)
    {
        return false;
    }

    const char *amount_ptr = strstr(json, "\"amount\":\"");
    if (!amount_ptr)
    {
        return false;
    }

    amount_ptr += 10;
    double price = strtod(amount_ptr, NULL);
    if (price <= 0.0)
    {
        return false;
    }

    *out_price = price;
    return true;
}

static void format_price_with_commas(long long value, char *out, size_t out_size)
{
    char temp[32];
    snprintf(temp, sizeof(temp), "%lld", value);

    int len = (int)strlen(temp);
    int commas = (len > 0) ? (len - 1) / 3 : 0;
    int out_len = len + commas;
    if (out_len + 1 > (int)out_size)
    {
        strncpy(out, temp, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    out[out_len] = '\0';
    int src = len - 1;
    int dst = out_len - 1;
    int group = 0;
    while (src >= 0)
    {
        out[dst--] = temp[src--];
        group++;
        if (group == 3 && src >= 0)
        {
            out[dst--] = ',';
            group = 0;
        }
    }
}

static void price_set_status(const char *status)
{
    if (!status)
    {
        return;
    }

    strncpy(current_price_status, status, sizeof(current_price_status) - 1);
    current_price_status[sizeof(current_price_status) - 1] = '\0';

    if (price_status_label)
    {
        lv_label_set_text(price_status_label, current_price_status);
    }
}

static void price_task(void *arg)
{
    (void)arg;
    while (1)
    {
        if (!price_wifi_connected())
        {
            if (lvgl_port_lock(50))
            {
                price_set_status("WAITING FOR WIFI");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!price_ensure_netif())
        {
            if (lvgl_port_lock(50))
            {
                price_set_status("NETIF ERROR");
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (lvgl_port_lock(50))
        {
            price_set_status("LOADING...");
            lvgl_port_unlock();
        }

        bool updated = price_fetch_once();
        if (lvgl_port_lock(50))
        {
            if (updated)
            {
                if (price_value_label)
                {
                    lv_label_set_text(price_value_label, current_price_text);
                }
                price_set_status("LIVE");
                lvgl_port_unlock();
                vTaskDelay(pdMS_TO_TICKS(PRICE_FETCH_INTERVAL_MS));
                continue;
            }

            price_set_status("RETRYING...");
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
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

static lv_obj_t *create_bottom_nav_btn_img(lv_obj_t *parent, const lv_img_dsc_t *img_dsc, lv_event_cb_t event_cb, bool active)
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

    lv_obj_t *img = lv_img_create(btn);
    lv_img_set_src(img, img_dsc);
    lv_obj_set_style_img_recolor(img, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_center(img);

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

void price_home_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    home_screen_create();
    lv_scr_load(home_get_screen());
    price_screen_destroy();
}

void price_block_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    block_screen_create();
    lv_scr_load(block_get_screen());
    price_screen_destroy();
}

void price_mempool_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    price_screen_destroy();
}

void price_clock_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    price_screen_destroy();
}

void price_wifi_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    wifi_screen_create();
    lv_scr_load(wifi_get_screen());
    price_screen_destroy();
}

void price_settings_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    price_screen_destroy();
}

void price_night_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    night_screen_create();
    lv_scr_load(night_get_screen());
    price_screen_destroy();
}
