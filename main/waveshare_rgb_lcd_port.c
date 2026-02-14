#include "waveshare_rgb_lcd_port.h"

static const char *TAG = "example";

static uint8_t current_brightness = LCD_BACKLIGHT_DEFAULT_BRIGHTNESS;
static bool backlight_initialized = false;

// VSYNC event callback function
IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return lvgl_port_notify_rgb_vsync();
}

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
/**
 * @brief I2C master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Configure I2C parameters
    i2c_param_config(i2c_master_port, &i2c_conf);

    // Install I2C driver
    return i2c_driver_install(i2c_master_port, i2c_conf.mode, 0, 0, 0);
}

// GPIO initialization
void gpio_init(void)
{
    // Zero-initialize the config structure
    gpio_config_t io_conf = {};
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);
}

// This translates the GPIO expander output byte into the ESP32 IO signals that need to be set
void set_ext_to_io(uint8_t value)
{
    uint8_t io_11_val, io_12_val, io_13_val;

    io_11_val = (value & 0x02) >> 1;
    io_12_val = (value & 0x04) >> 2;
    io_13_val = (value & 0x08) >> 3;

    gpio_set_level(GPIO_NUM_11, io_11_val);
    gpio_set_level(EXAMPLE_LCD_IO_RGB_DISP, io_12_val);
    gpio_set_level(EXAMPLE_LCD_IO_RST, io_13_val);

}


// Reset the touch screen
void waveshare_esp32_s3_touch_reset()
{
    //uint8_t write_buf = 0x01;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    // Reset the touch screen. It is recommended to reset the touch screen before using it.
    //write_buf = 0x2C;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    set_ext_to_io(0x2C);

    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GPIO_INPUT_IO_4, 0);
    esp_rom_delay_us(100 * 1000);

    //write_buf = 0x2E;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    set_ext_to_io(0x2E);
    esp_rom_delay_us(200 * 1000);
}

#endif

static esp_err_t init_gt911_touch_with_fallback(esp_lcd_touch_handle_t *out_tp_handle)
{
    esp_lcd_touch_handle_t tp_handle = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    // Try default GT911 address first (0x5D), then backup (0x14).
    const uint8_t gt911_addrs[] = {
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP,
    };

    for (size_t i = 0; i < sizeof(gt911_addrs) / sizeof(gt911_addrs[0]); i++) {
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        tp_io_config.dev_addr = gt911_addrs[i];

        ESP_LOGI(TAG, "Initialize I2C panel IO for GT911 at 0x%02X", (unsigned int)tp_io_config.dev_addr);
        esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "GT911 I2C IO init failed at 0x%02X: %s", (unsigned int)tp_io_config.dev_addr, esp_err_to_name(ret));
            continue;
        }

        esp_lcd_touch_io_gt911_config_t gt911_cfg = {
            .dev_addr = tp_io_config.dev_addr,
        };
        const esp_lcd_touch_config_t tp_cfg = {
            .x_max = EXAMPLE_LCD_H_RES,
            .y_max = EXAMPLE_LCD_V_RES,
            .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
            .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
            .driver_data = &gt911_cfg,
        };

        ESP_LOGI(TAG, "Initialize touch controller GT911 at 0x%02X", (unsigned int)tp_io_config.dev_addr);
        ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle);
        if (ret == ESP_OK) {
            *out_tp_handle = tp_handle;
            return ESP_OK;
        }

        ESP_LOGW(TAG, "GT911 init failed at 0x%02X: %s", (unsigned int)tp_io_config.dev_addr, esp_err_to_name(ret));
    }

    *out_tp_handle = NULL;
    return ESP_FAIL;
}

// Initialize RGB LCD
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
{
    ESP_LOGI(TAG, "Install RGB LCD panel driver"); // Log the start of the RGB LCD panel driver installation
    esp_lcd_panel_handle_t panel_handle = NULL; // Declare a handle for the LCD panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // Set the clock source for the panel
        .timings =  {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency
            .h_res = EXAMPLE_LCD_H_RES, // Horizontal resolution
            .v_res = EXAMPLE_LCD_V_RES, // Vertical resolution
            .hsync_pulse_width = 4, // Horizontal sync pulse width
            .hsync_back_porch = 8, // Horizontal back porch
            .hsync_front_porch = 8, // Horizontal front porch
            .vsync_pulse_width = 4, // Vertical sync pulse width
            .vsync_back_porch = 8, // Vertical back porch
            .vsync_front_porch = 8, // Vertical front porch
            .flags = {
                .pclk_active_neg = 1, // Active low pixel clock
            },
        },
        .data_width = EXAMPLE_RGB_DATA_WIDTH, // Data width for RGB
        .bits_per_pixel = EXAMPLE_RGB_BIT_PER_PIXEL, // Bits per pixel
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS, // Number of frame buffers
        .bounce_buffer_size_px = EXAMPLE_RGB_BOUNCE_BUFFER_SIZE, // Bounce buffer size in pixels
        .sram_trans_align = 4, // SRAM transaction alignment
        .psram_trans_align = 64, // PSRAM transaction alignment
        .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC, // GPIO number for horizontal sync
        .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC, // GPIO number for vertical sync
        .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE, // GPIO number for data enable
        .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK, // GPIO number for pixel clock
        .disp_gpio_num = EXAMPLE_LCD_IO_RGB_DISP, // GPIO12 still needed for display enable
        .data_gpio_nums = {
            EXAMPLE_LCD_IO_RGB_DATA0,
            EXAMPLE_LCD_IO_RGB_DATA1,
            EXAMPLE_LCD_IO_RGB_DATA2,
            EXAMPLE_LCD_IO_RGB_DATA3,
            EXAMPLE_LCD_IO_RGB_DATA4,
            EXAMPLE_LCD_IO_RGB_DATA5,
            EXAMPLE_LCD_IO_RGB_DATA6,
            EXAMPLE_LCD_IO_RGB_DATA7,
            EXAMPLE_LCD_IO_RGB_DATA8,
            EXAMPLE_LCD_IO_RGB_DATA9,
            EXAMPLE_LCD_IO_RGB_DATA10,
            EXAMPLE_LCD_IO_RGB_DATA11,
            EXAMPLE_LCD_IO_RGB_DATA12,
            EXAMPLE_LCD_IO_RGB_DATA13,
            EXAMPLE_LCD_IO_RGB_DATA14,
            EXAMPLE_LCD_IO_RGB_DATA15,
        },
        .flags = {
            .fb_in_psram = 1, // Use PSRAM for framebuffer
        },
    };

    // Create a new RGB panel with the specified configuration
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel"); // Log the initialization of the RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); // Initialize the LCD panel

    esp_lcd_touch_handle_t tp_handle = NULL; // Declare a handle for the touch panel
#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus"); // Log the initialization of the I2C bus
    ESP_ERROR_CHECK(i2c_master_init()); // Initialize the I2C master
    ESP_LOGI(TAG, "Initialize GPIO"); // Log GPIO initialization
    gpio_init(); // Initialize GPIO pins
    ESP_LOGI(TAG, "Initialize Touch LCD"); // Log touch LCD initialization
    waveshare_esp32_s3_touch_reset(); // Reset the touch panel

    esp_err_t tp_ret = init_gt911_touch_with_fallback(&tp_handle);
    if (tp_ret != ESP_OK) {
        ESP_LOGE(TAG, "GT911 touch init failed on all known addresses, continuing without touch");
    }
#endif // CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle)); // Initialize LVGL with the panel and touch handles

    // Register callbacks for RGB panel events
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
#if EXAMPLE_RGB_BOUNCE_BUFFER_SIZE > 0
        .on_bounce_frame_finish = rgb_lcd_on_vsync_event, // Callback for bounce frame finish
#else
        .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL)); // Register event callbacks

    // Initialize GPIO15 PWM for TPS61161 brightness control
    ESP_LOGI(TAG, "=== INITIALIZING TPS61161 BRIGHTNESS CONTROL ===");
    ESP_LOGI(TAG, "GPIO12: Display enable (ESP-IDF LCD driver - always HIGH)");
    ESP_LOGI(TAG, "GPIO15: PWM brightness control (disconnected from GPIO12 conflict)");
    ESP_LOGI(TAG, "PWM Frequency: %d Hz (datasheet optimal: 5kHz-100kHz)", LCD_BACKLIGHT_LEDC_FREQUENCY);
    ESP_LOGI(TAG, "Default Brightness: %d%%", LCD_BACKLIGHT_DEFAULT_BRIGHTNESS);
    
    esp_err_t pwm_ret = lcd_backlight_pwm_init();
    if (pwm_ret != ESP_OK) {
        ESP_LOGE(TAG, "=== PWM BRIGHTNESS CONTROL FAILED ===");
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(pwm_ret), pwm_ret);
        ESP_LOGE(TAG, "TPS61161 brightness control not available");
        backlight_initialized = false;
    } else {
        ESP_LOGI(TAG, "=== PWM BRIGHTNESS CONTROL SUCCESSFUL ===");
        ESP_LOGI(TAG, "GPIO15 → TPS61161 CTRL: Clean PWM signal");
        ESP_LOGI(TAG, "GPIO12 → Display enable: Handled by ESP-IDF");
        ESP_LOGI(TAG, "Current brightness: %d%%", lcd_backlight_get_brightness());
    }

    return ESP_OK; // Return success
}

/******************************* Turn on the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_on()
{
    //Configure CH422G to output mode
    //uint8_t write_buf = 0x01;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    //Pull the backlight pin high to light the screen backlight
    //write_buf = 0x1E;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    // Enable TPS61161 brightness control via GPIO15 PWM (GPIO12 disconnected)
    if (backlight_initialized) {
        ESP_LOGI(TAG, "Enabling TPS61161 brightness control via GPIO15 PWM");
        lcd_backlight_enable();
    } else {
        ESP_LOGI(TAG, "PWM brightness control not available - TPS61161 may not respond");
    }

    return ESP_OK;
}

/******************************* Turn off the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_off()
{
    //Configure CH422G to output mode
    //uint8_t write_buf = 0x01;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    //Turn off the screen backlight by pulling the backlight pin low
    //write_buf = 0x1A;
    //i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    // Turn off TPS61161 brightness control via GPIO15 PWM
    if (backlight_initialized) {
        ESP_LOGI(TAG, "Disabling TPS61161 brightness control via GPIO15 PWM");
        lcd_backlight_disable();
    } else {
        ESP_LOGI(TAG, "PWM brightness control not available");
    }
    
    // Note: With GPIO12 disconnected, GPIO15 PWM fully controls TPS61161

    return ESP_OK;
}

/******************************* PWM Brightness Control Functions **************************************/

/**
 * @brief Initialize PWM for TPS61161 backlight brightness control
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_backlight_pwm_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing GPIO12 PWM for TPS61161 backlight driver");
    ESP_LOGI(TAG, "GPIO12 will control both display enable and brightness");

    // Configure LEDC timer for TPS61161 (20kHz PWM frequency)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LCD_BACKLIGHT_LEDC_MODE,
        .timer_num        = LCD_BACKLIGHT_LEDC_TIMER,
        .duty_resolution  = LCD_BACKLIGHT_LEDC_DUTY_RES,
        .freq_hz          = LCD_BACKLIGHT_LEDC_FREQUENCY,  // 20kHz for TPS61161
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        backlight_initialized = false;
        return ret;
    }
    
    // Configure LEDC channel for TPS61161 - start with default brightness
    uint32_t default_duty = (LCD_BACKLIGHT_MAX_DUTY * LCD_BACKLIGHT_DEFAULT_BRIGHTNESS) / 100;
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LCD_BACKLIGHT_LEDC_MODE,
        .channel        = LCD_BACKLIGHT_LEDC_CHANNEL,
        .timer_sel      = LCD_BACKLIGHT_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LCD_BACKLIGHT_PWM_GPIO,
        .duty           = default_duty, // Start with default brightness, not 0
        .hpoint         = 0
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        backlight_initialized = false;
        return ret;
    }

    // Install fade service for smooth transitions (optional)
    ret = ledc_fade_func_install(0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to install LEDC fade service: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Fade functions will not be available");
        // Don't fail initialization for fade service
    }

    backlight_initialized = true;
    current_brightness = LCD_BACKLIGHT_DEFAULT_BRIGHTNESS;
    ESP_LOGI(TAG, "PWM backlight initialized on GPIO%d with %d%% brightness",
             LCD_BACKLIGHT_PWM_GPIO, LCD_BACKLIGHT_DEFAULT_BRIGHTNESS);

    return ESP_OK;
}

/**
 * @brief Set backlight brightness using GPIO15 (TPS61161 control)
 * @param brightness_percent Brightness level (0-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_backlight_set_brightness(uint8_t brightness_percent)
{
    if (!backlight_initialized) {
        ESP_LOGE(TAG, "PWM backlight not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (brightness_percent > 100) {
        brightness_percent = 100;
    }

    // TPS61161 brightness control via GPIO15 PWM (GPIO12 handles display enable)
    uint32_t duty;
    if (brightness_percent == 0) {
        // 0% brightness = PWM off
        duty = 0;
        ESP_LOGI(TAG, "GPIO15 PWM brightness 0%% -> PWM OFF (display stays enabled via GPIO12)");
    } else {
        // Map 1-100% to minimum-maximum duty cycle for TPS61161
        uint32_t min_duty = LCD_BACKLIGHT_MIN_DUTY;
        uint32_t max_duty = LCD_BACKLIGHT_MAX_DUTY;
        uint32_t duty_range = max_duty - min_duty;
        duty = min_duty + ((duty_range * brightness_percent) / 100);
        ESP_LOGI(TAG, "GPIO15 PWM brightness %d%% -> duty: %lu (range: %lu-%lu)",
                 brightness_percent, duty, min_duty, max_duty);
    }

    esp_err_t ret = ledc_set_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDC duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = ledc_update_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update LEDC duty: %s", esp_err_to_name(ret));
        return ret;
    }

    current_brightness = brightness_percent;
    ESP_LOGI(TAG, "TPS61161 backlight brightness set to %d%% on GPIO%d (duty: %lu)", brightness_percent, LCD_BACKLIGHT_PWM_GPIO, duty);

    return ESP_OK;
}

/**
 * @brief Get current backlight brightness
 * @return Current brightness level (0-100%)
 */
uint8_t lcd_backlight_get_brightness(void)
{
    return current_brightness;
}

/**
 * @brief Fade backlight to target brightness
 * @param target_brightness Target brightness level (0-100%)
 * @param fade_time_ms Fade duration in milliseconds
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_backlight_fade_to(uint8_t target_brightness, uint32_t fade_time_ms)
{
    if (!backlight_initialized) {
        ESP_LOGE(TAG, "PWM backlight not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (target_brightness > 100) {
        target_brightness = 100;
    }

    // Calculate target duty cycle
    uint32_t target_duty = (LCD_BACKLIGHT_MAX_DUTY * target_brightness) / 100;

    // Install fade function
    esp_err_t ret = ledc_set_fade_with_time(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL,
                                           target_duty, fade_time_ms);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDC fade: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start fade
    ret = ledc_fade_start(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LEDC fade: %s", esp_err_to_name(ret));
        return ret;
    }

    current_brightness = target_brightness;
    ESP_LOGI(TAG, "Fading backlight to %d%% over %lums", target_brightness, fade_time_ms);

    return ESP_OK;
}

/**
 * @brief Enable backlight (set to current brightness)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_backlight_enable(void)
{
    if (!backlight_initialized) {
        ESP_LOGE(TAG, "PWM backlight not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    return lcd_backlight_set_brightness(current_brightness);
}

/**
 * @brief Disable backlight (turn off PWM completely)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_backlight_disable(void)
{
    if (!backlight_initialized) {
        ESP_LOGE(TAG, "PWM backlight not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Turn off PWM completely (GPIO15 independent of display enable)
    esp_err_t ret = ledc_set_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDC duty to 0: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = ledc_update_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update LEDC duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "TPS61161 backlight disabled (PWM OFF on GPIO%d)", LCD_BACKLIGHT_PWM_GPIO);
    return ESP_OK;
}

/******************************* Example code **************************************/
static void draw_event_cb(lv_event_t *e) // Draw event callback function 
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e); // Get the draw part descriptor 
    if (dsc->part == LV_PART_ITEMS)
    {                                                                 // If drawing chart items 
        lv_obj_t *obj = lv_event_get_target(e);                       // Get the target object of the event 
        lv_chart_series_t *ser = lv_chart_get_series_next(obj, NULL); // Get the series of the chart 
        uint32_t cnt = lv_chart_get_point_count(obj);                 // Get the number of points in the chart 
        /* Make older values more transparent */
        dsc->rect_dsc->bg_opa = (LV_OPA_COVER * dsc->id) / (cnt - 1); // Set opacity based on the index 

        /* Make smaller values blue, higher values red  */
        lv_coord_t *x_array = lv_chart_get_x_array(obj, ser); // Get the X-axis array 
        lv_coord_t *y_array = lv_chart_get_y_array(obj, ser); // Get the Y-axis array 
        /* dsc->id is the drawing order, but we need the index of the point being drawn dsc->id  */
        uint32_t start_point = lv_chart_get_x_start_point(obj, ser); // Get the start point of the chart 
        uint32_t p_act = (start_point + dsc->id) % cnt;              // Calculate the actual index based on the start point 
        lv_opa_t x_opa = (x_array[p_act] * LV_OPA_50) / 200;         // Calculate X-axis opacity 
        lv_opa_t y_opa = (y_array[p_act] * LV_OPA_50) / 1000;        // Calculate Y-axis opacity 

        dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_RED), // Mix colors 
                                               lv_palette_main(LV_PALETTE_BLUE),
                                               x_opa + y_opa);
    }
}

static void add_data(lv_timer_t *timer) // Timer callback to add data to the chart 
{
    lv_obj_t *chart = timer->user_data;                                                                        // Get the chart associated with the timer 
    lv_chart_set_next_value2(chart, lv_chart_get_series_next(chart, NULL), lv_rand(0, 200), lv_rand(0, 1000)); // Add random data to the chart 
}

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#scatter-chart
void example_lvgl_demo_ui() // LVGL demo UI initialization function 
{
    lv_obj_t *scr = lv_scr_act();                                              // Get the current active screen 
    lv_obj_t *chart = lv_chart_create(scr);                                    // Create a chart object 
    lv_obj_set_size(chart, 200, 150);                                          // Set chart size 
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);                                // Center the chart on the screen 
    lv_obj_add_event_cb(chart, draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); // Add draw event callback 
    lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);                      /* Remove chart lines  */

    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER); // Set chart type to scatter 

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 5, 5, 5, 1, true, 30);  // Set X-axis ticks 
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 50); // Set Y-axis ticks 

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 200);  // Set X-axis range 
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000); // Set Y-axis range 

    lv_chart_set_point_count(chart, 50); // Set the number of points in the chart 

    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); // Add a series to the chart 
    for (int i = 0; i < 50; i++)
    {                                                                            // Add random points to the chart 
        lv_chart_set_next_value2(chart, ser, lv_rand(0, 200), lv_rand(0, 1000)); // Set X and Y values 
    }

    lv_timer_create(add_data, 100, chart); // Create a timer to add new data every 100ms 
}
