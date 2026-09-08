#include "theme.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define THEME_NVS_NAMESPACE "settings"
#define THEME_NVS_ACCENT_KEY "accent"

static const char *TAG = "theme";
static accent_theme_t current_accent = ACCENT_THEME_DEFAULT;
static bool initialized = false;

void theme_initialize(void)
{
    if (initialized) {
        return;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not initialize theme storage: %s", esp_err_to_name(err));
        initialized = true;
        return;
    }

    nvs_handle_t handle;
    err = nvs_open(THEME_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        uint8_t saved_accent = ACCENT_THEME_DEFAULT;
        if (nvs_get_u8(handle, THEME_NVS_ACCENT_KEY, &saved_accent) == ESP_OK &&
            theme_palette_is_valid((accent_theme_t)saved_accent)) {
            current_accent = (accent_theme_t)saved_accent;
        }
        nvs_close(handle);
    }

    initialized = true;
}

accent_theme_t theme_get_accent(void)
{
    theme_initialize();
    return current_accent;
}

esp_err_t theme_set_accent(accent_theme_t theme)
{
    if (!theme_palette_is_valid(theme)) {
        return ESP_ERR_INVALID_ARG;
    }

    theme_initialize();
    nvs_handle_t handle;
    esp_err_t err = nvs_open(THEME_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u8(handle, THEME_NVS_ACCENT_KEY, (uint8_t)theme);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err == ESP_OK) {
        current_accent = theme;
    }
    return err;
}

lv_color_t theme_get_accent_color(void)
{
    return lv_color_hex(theme_palette_accent_hex(theme_get_accent()));
}

lv_color_t theme_get_text_on_accent_color(void)
{
    return lv_color_hex(theme_palette_text_hex(theme_get_accent()));
}
