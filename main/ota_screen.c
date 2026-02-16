/**
 * @file ota_screen.c
 *
 * Minimal OTA screen implementation - reduces LVGL updates during flash writes
 */

#include "ota_screen.h"
#include "custom_fonts.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ota_screen";

static lv_obj_t *ota_screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *progress_label = NULL;
static int last_reported_progress = -1;

void ota_screen_show(void)
{
    ESP_LOGI(TAG, "Creating OTA update screen");

    // Create new screen
    ota_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ota_screen, lv_color_hex(0x000000), 0);

    // Title
    lv_obj_t *title = lv_label_create(ota_screen);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -80);

    // Status label
    status_label = lv_label_create(ota_screen);
    lv_label_set_text(status_label, "Downloading...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -20);

    // Progress label
    progress_label = lv_label_create(ota_screen);
    lv_label_set_text(progress_label, "0%");
    lv_obj_set_style_text_color(progress_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_48, 0);
    lv_obj_align(progress_label, LV_ALIGN_CENTER, 0, 40);

    // Warning message
    lv_obj_t *warning = lv_label_create(ota_screen);
    lv_label_set_text(warning, "Do not power off device");
    lv_obj_set_style_text_color(warning, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(warning, &lv_font_montserrat_18, 0);
    lv_obj_align(warning, LV_ALIGN_BOTTOM_MID, 0, -40);

    // Load the screen
    lv_scr_load(ota_screen);

    last_reported_progress = 0;
    ESP_LOGI(TAG, "OTA screen displayed");
}

void ota_screen_update_progress(int progress)
{
    if (!ota_screen || !progress_label) {
        return;
    }

    // Only update every 25% to minimize LVGL rendering during flash writes
    int progress_step = (progress / 25) * 25;
    if (progress >= 100) {
        progress_step = 100;
    }

    if (progress_step != last_reported_progress) {
        ESP_LOGI(TAG, "Updating progress display: %d%%", progress_step);

        // Quick update without animation
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", progress_step);
        lv_label_set_text(progress_label, buf);

        // Update status
        if (progress_step >= 100) {
            lv_label_set_text(status_label, "Complete! Rebooting...");
        } else if (progress_step >= 50) {
            lv_label_set_text(status_label, "Flashing firmware...");
        }

        last_reported_progress = progress_step;
    }
}

void ota_screen_show_error(const char *error)
{
    if (!ota_screen || !status_label) {
        return;
    }

    ESP_LOGE(TAG, "OTA Error: %s", error);

    lv_label_set_text(status_label, "Update Failed!");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0);

    if (progress_label) {
        lv_label_set_text(progress_label, error);
        lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_18, 0);
    }
}

void ota_screen_hide(void)
{
    if (ota_screen) {
        lv_obj_del(ota_screen);
        ota_screen = NULL;
        status_label = NULL;
        progress_label = NULL;
        last_reported_progress = -1;
    }
}
