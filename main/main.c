#include "waveshare_rgb_lcd_port.h"
#include "loading.h"
#include "display_control.h"
#include "settings.h"
#include "wifi.h"
#include "price.h"
#include "mempool.h"

static const char *TAG = "main";

void app_main()
{
    waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();
    settings_initialize();
    
    ESP_LOGI(TAG, "BAP Touch Display -- Build by WantClue with Love");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        esp_err_t wifi_ret = wifi_start_saved_connection();
        if (wifi_ret != ESP_OK && wifi_ret != ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "Saved WiFi startup failed: %s", esp_err_to_name(wifi_ret));
        }
        // Start existing cache services without waiting for a screen visit.
        price_service_start();
        mempool_service_start();
        // screen init
        loading();
        ESP_ERROR_CHECK(display_control_init());
        
        // Release the mutex
        lvgl_port_unlock();
    }
}
