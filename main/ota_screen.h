/**
 * @file ota_screen.h
 *
 * Minimal OTA update screen with reduced LVGL activity
 */

#ifndef OTA_SCREEN_H
#define OTA_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl_port.h"

/**
 * @brief Show OTA update screen (call once before OTA starts)
 */
void ota_screen_show(void);

/**
 * @brief Update progress (updates only every 25% to minimize PSRAM writes)
 * @param progress Progress percentage 0-100
 */
void ota_screen_update_progress(int progress);

/**
 * @brief Show error message on OTA screen
 */
void ota_screen_show_error(const char *error);

/**
 * @brief Hide OTA screen and return to normal operation
 */
void ota_screen_hide(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_SCREEN_H */
