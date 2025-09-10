#ifndef _RGB_LCD_H_
#define _RGB_LCD_H_

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "lv_demos.h"
#include "lvgl_port.h"

#define CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911 1 // 1 initiates the touch, 0 closes the touch.

#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4
#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_IO_4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_H_RES               (LVGL_PORT_H_RES)
#define EXAMPLE_LCD_V_RES               (LVGL_PORT_V_RES)
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (16 * 1000 * 1000)
#define EXAMPLE_LCD_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_DATA_WIDTH          (16)
#define EXAMPLE_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_H_RES * CONFIG_EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT)
#define EXAMPLE_LCD_IO_RGB_DISP         (GPIO_NUM_12)   // -1 if not used
#define EXAMPLE_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)
#define EXAMPLE_LCD_IO_RGB_HSYNC        (GPIO_NUM_46)
#define EXAMPLE_LCD_IO_RGB_DE           (GPIO_NUM_5)
#define EXAMPLE_LCD_IO_RGB_PCLK         (GPIO_NUM_7)
#define EXAMPLE_LCD_IO_RGB_DATA0        (GPIO_NUM_14)
#define EXAMPLE_LCD_IO_RGB_DATA1        (GPIO_NUM_38)
#define EXAMPLE_LCD_IO_RGB_DATA2        (GPIO_NUM_18)
#define EXAMPLE_LCD_IO_RGB_DATA3        (GPIO_NUM_17)
#define EXAMPLE_LCD_IO_RGB_DATA4        (GPIO_NUM_10)
#define EXAMPLE_LCD_IO_RGB_DATA5        (GPIO_NUM_39)
#define EXAMPLE_LCD_IO_RGB_DATA6        (GPIO_NUM_0)
#define EXAMPLE_LCD_IO_RGB_DATA7        (GPIO_NUM_45)
#define EXAMPLE_LCD_IO_RGB_DATA8        (GPIO_NUM_48)
#define EXAMPLE_LCD_IO_RGB_DATA9        (GPIO_NUM_47)
#define EXAMPLE_LCD_IO_RGB_DATA10       (GPIO_NUM_21)
#define EXAMPLE_LCD_IO_RGB_DATA11       (GPIO_NUM_1)
#define EXAMPLE_LCD_IO_RGB_DATA12       (GPIO_NUM_2)
#define EXAMPLE_LCD_IO_RGB_DATA13       (GPIO_NUM_42)
#define EXAMPLE_LCD_IO_RGB_DATA14       (GPIO_NUM_41)
#define EXAMPLE_LCD_IO_RGB_DATA15       (GPIO_NUM_40)

#define EXAMPLE_LCD_IO_RST              (GPIO_NUM_13)    // -1 if not used
#define EXAMPLE_PIN_NUM_BK_LIGHT        (-1)    // -1 if not used
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL   (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL  !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

// Backlight Control definitions (GPIO15 - LED_PWM pin)
#define LCD_BACKLIGHT_PWM_GPIO          (GPIO_NUM_15)   // GPIO15 for brightness control
#define LCD_BACKLIGHT_LEDC_TIMER        LEDC_TIMER_0
#define LCD_BACKLIGHT_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LCD_BACKLIGHT_LEDC_CHANNEL      LEDC_CHANNEL_0
#define LCD_BACKLIGHT_LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define LCD_BACKLIGHT_LEDC_FREQUENCY    (25000)         // 25kHz PWM frequency for TPS61161 (optimal range: 5kHz-100kHz)
#define LCD_BACKLIGHT_MAX_DUTY          ((1 << LCD_BACKLIGHT_LEDC_DUTY_RES) - 1)
#define LCD_BACKLIGHT_MIN_DUTY          (10)            // Minimum duty for TPS61161
#define LCD_BACKLIGHT_DEFAULT_BRIGHTNESS (100)           // Default 80% brightness

#define EXAMPLE_PIN_NUM_TOUCH_RST       (-1)            // -1 if not used
#define EXAMPLE_PIN_NUM_TOUCH_INT       (-1)            // -1 if not used

bool example_lvgl_lock(int timeout_ms);
void example_lvgl_unlock(void);

void set_ext_to_io(uint8_t value);

esp_err_t waveshare_esp32_s3_rgb_lcd_init();

// Enhanced backlight control functions
esp_err_t wavesahre_rgb_lcd_bl_on();
esp_err_t wavesahre_rgb_lcd_bl_off();

// New brightness control functions
esp_err_t lcd_backlight_pwm_init(void);
esp_err_t lcd_backlight_set_brightness(uint8_t brightness_percent);
uint8_t lcd_backlight_get_brightness(void);
esp_err_t lcd_backlight_fade_to(uint8_t target_brightness, uint32_t fade_time_ms);
esp_err_t lcd_backlight_enable(void);
esp_err_t lcd_backlight_disable(void);

void example_lvgl_demo_ui();

#endif