#include "waveshare_rgb_lcd_port.h"
#include "loading.h"

static const char *TAG = "main";

void app_main()
{
    waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();
    
    ESP_LOGI(TAG, "BAP Touch Display -- Build by WantClue with Love");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // screen init
        loading();
        
        // Release the mutex
        lvgl_port_unlock();
    }
}
