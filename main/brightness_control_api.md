# TPS61161 Backlight Brightness Control API

## Overview

This implementation provides PWM-based brightness control for the ST7701 LCD screen using the TPS61161 backlight driver chip. The system uses dual GPIO control with the ESP32-S3's LEDC peripheral for precise brightness adjustment.

## Hardware Configuration

### Final Working Setup
- **Display Enable**: GPIO12 (controlled by ESP-IDF LCD driver - always HIGH)
- **Brightness Control**: GPIO15 (PWM to TPS61161 CTRL pin)
- **PWM Frequency**: 25kHz (TPS61161 optimal range: 5kHz-100kHz)
- **Resolution**: 10-bit (1024 levels)
- **Brightness Range**: 0-100% (full range)
- **Default Brightness**: 100%

### Hardware Discovery
The key breakthrough was resolving a GPIO signal conflict:
- **Problem**: Both GPIO12 and GPIO15 were connected to TPS61161 CTRL pin
- **Solution**: Disconnect GPIO12 from TPS61161 CTRL (keep for display enable only)
- **Result**: Clean PWM signal from GPIO15 to TPS61161 without interference

## TPS61161 Backlight Driver

### Chip Specifications
- **PWM Frequency Range**: 5kHz to 100kHz (per datasheet)
- **Control Method**: PWM duty cycle on CTRL pin
- **EasyScale Detection**: Requires minimum frequency for proper operation
- **Logic-Only Pin**: No external RC filtering needed

### GPIO Configuration
```
GPIO12 (DISP) → Display Enable (ESP-IDF LCD driver)
GPIO15 (PWM)  → TPS61161 CTRL (Our PWM implementation)
```

## API Functions

### Initialization

```c
esp_err_t lcd_backlight_pwm_init(void);
```
- **Description**: Initialize PWM for TPS61161 backlight brightness control
- **Returns**: ESP_OK on success, error code otherwise
- **Note**: Called automatically during `waveshare_esp32_s3_rgb_lcd_init()`
- **Hardware**: Configures GPIO15 for 25kHz PWM output to TPS61161 CTRL

### Brightness Control

```c
esp_err_t lcd_backlight_set_brightness(uint8_t brightness_percent);
```
- **Description**: Set backlight brightness immediately via TPS61161
- **Parameters**: 
  - `brightness_percent`: Brightness level (0-100%)
- **Returns**: ESP_OK on success, error code otherwise
- **Hardware**: Controls TPS61161 via PWM duty cycle on GPIO15
- **Example**: `lcd_backlight_set_brightness(75);` // Set to 75%

```c
uint8_t lcd_backlight_get_brightness(void);
```
- **Description**: Get current backlight brightness
- **Returns**: Current brightness level (0-100%)
- **Example**: `uint8_t current = lcd_backlight_get_brightness();`

### Smooth Transitions

```c
esp_err_t lcd_backlight_fade_to(uint8_t target_brightness, uint32_t fade_time_ms);
```
- **Description**: Fade backlight to target brightness over specified time
- **Parameters**:
  - `target_brightness`: Target brightness level (0-100%)
  - `fade_time_ms`: Fade duration in milliseconds
- **Returns**: ESP_OK on success, error code otherwise
- **Hardware**: Uses ESP32-S3 LEDC fade functionality
- **Example**: `lcd_backlight_fade_to(50, 2000);` // Fade to 50% over 2 seconds

### Enable/Disable

```c
esp_err_t lcd_backlight_enable(void);
```
- **Description**: Enable backlight (set to current brightness)
- **Returns**: ESP_OK on success, error code otherwise
- **Note**: Display remains enabled via GPIO12

```c
esp_err_t lcd_backlight_disable(void);
```
- **Description**: Disable backlight (set to 0% brightness)
- **Returns**: ESP_OK on success, error code otherwise
- **Note**: Display remains enabled via GPIO12, only brightness goes to 0%

### Legacy Functions (Enhanced)

```c
esp_err_t wavesahre_rgb_lcd_bl_on(void);
```
- **Description**: Turn on screen backlight
- **Implementation**: Enables PWM brightness control if available
- **Fallback**: Uses digital control if PWM initialization failed

```c
esp_err_t wavesahre_rgb_lcd_bl_off(void);
```
- **Description**: Turn off screen backlight
- **Implementation**: Disables PWM brightness control
- **Note**: Display enable (GPIO12) remains active

## Usage Examples

### Basic Brightness Control

```c
// Set brightness to 60%
lcd_backlight_set_brightness(60);

// Get current brightness
uint8_t brightness = lcd_backlight_get_brightness();
ESP_LOGI(TAG, "Current brightness: %d%%", brightness);
```

### Smooth Fade Effects

```c
// Fade to dim (20%) over 1 second
lcd_backlight_fade_to(20, 1000);

// Wait for fade to complete
vTaskDelay(pdMS_TO_TICKS(1500));

// Fade back to bright (90%) over 2 seconds
lcd_backlight_fade_to(90, 2000);
```

### Power Management

```c
// Temporarily disable backlight (screen stays on)
lcd_backlight_disable();

// Later, re-enable at previous brightness
lcd_backlight_enable();
```

### Settings Integration

The brightness control is integrated with the settings UI:

```c
// Called from settings slider (settings.c)
void settings_brightness_slider_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    current_settings.brightness_percent = lv_slider_get_value(slider);
    
    // Apply brightness change immediately
    lcd_backlight_set_brightness(current_settings.brightness_percent);
    
    printf("Screen brightness set to: %d%%\n", current_settings.brightness_percent);
}
```

## Configuration Constants

Located in `waveshare_rgb_lcd_port.h`:

```c
#define LCD_BACKLIGHT_PWM_GPIO          (GPIO_NUM_15)   // GPIO15 for brightness control
#define LCD_BACKLIGHT_LEDC_TIMER        LEDC_TIMER_0
#define LCD_BACKLIGHT_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LCD_BACKLIGHT_LEDC_CHANNEL      LEDC_CHANNEL_0
#define LCD_BACKLIGHT_LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define LCD_BACKLIGHT_LEDC_FREQUENCY    (25000)         // 25kHz PWM frequency for TPS61161
#define LCD_BACKLIGHT_MAX_DUTY          ((1 << LCD_BACKLIGHT_LEDC_DUTY_RES) - 1)
#define LCD_BACKLIGHT_MIN_DUTY          (10)            // Minimum duty for TPS61161
#define LCD_BACKLIGHT_DEFAULT_BRIGHTNESS (100)          // Default 100% brightness
```

## Error Handling

All functions return `esp_err_t` status codes:

- `ESP_OK`: Success
- `ESP_ERR_INVALID_STATE`: PWM not initialized
- Other ESP-IDF error codes for hardware failures

Always check return values in production code:

```c
esp_err_t ret = lcd_backlight_set_brightness(75);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set brightness: %s", esp_err_to_name(ret));
}
```

## Troubleshooting

### Common Issues

1. **Screen not visible**: Ensure GPIO12 is connected for display enable
2. **Brightness not working**: Check GPIO15 connection to TPS61161 CTRL
3. **Erratic behavior**: Verify no GPIO conflicts (both pins to same TPS61161 pin)

### Hardware Debugging

```c
// Check if PWM is initialized
if (backlight_initialized) {
    ESP_LOGI(TAG, "PWM brightness control active");
} else {
    ESP_LOGW(TAG, "PWM brightness control failed - using fallback");
}
```

### Signal Verification

- **GPIO12**: Should be HIGH (3.3V) for display enable
- **GPIO15**: Should show 25kHz PWM signal varying with brightness
- **TPS61161 CTRL**: Should receive clean PWM from GPIO15 only

## Power Consumption

- **Lower brightness = Lower power consumption**
- **PWM duty cycle directly controls TPS61161 LED current**
- **Recommended ranges**:
  - **Night mode**: 10-30% brightness
  - **Indoor use**: 40-70% brightness  
  - **Outdoor mode**: 80-100% brightness

## Thread Safety

- All brightness control functions are thread-safe
- Can be called from any FreeRTOS task
- LVGL integration uses proper mutex protection
- No additional locking required

## Compatibility

- **Backward Compatible**: Existing `wavesahre_rgb_lcd_bl_on/off()` functions enhanced
- **Fallback**: If PWM initialization fails, falls back to digital on/off control
- **Hardware**: Requires TPS61161 backlight driver with GPIO15 connected to CTRL pin
- **Display**: Works with ST7701 LCD controller and ESP32-S3 RGB LCD interface

## Performance

- **Response Time**: Immediate brightness changes (< 1ms)
- **Fade Smoothness**: Hardware-accelerated LEDC fade functions
- **Frequency**: 25kHz eliminates visible flicker and audible noise
- **Resolution**: 10-bit PWM provides 1024 brightness levels