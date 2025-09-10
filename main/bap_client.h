/**
 * @file bap_client.h
 * @brief BAP client main interface
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BAP client
 * Creates UART tasks and initializes communication
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_init(void);

/**
 * @brief Subscribe to a parameter
 * @param parameter Parameter name to subscribe to
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_subscribe(const char *parameter);

/**
 * @brief Request a parameter value
 * @param parameter Parameter name to request
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_request(const char *parameter);

/**
 * @brief Send frequency setting
 * @param frequency_mhz Frequency in MHz
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_send_frequency_setting(float frequency_mhz);

/**
 * @brief Send ASIC voltage setting
 * @param voltage Voltage value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_send_asic_voltage(float voltage);

/**
 * @brief Send fan speed setting
 * @param speed_percent Fan speed percentage (0-100)
 * @return ESP_OK on success, error code otherwise
 * 
 */
esp_err_t bap_client_send_fan_speed(int speed_percent);

/**
 * @brief Send automatic fan control setting
 * @param enabled true to enable, false to disable
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_send_automatic_fan_control(bool enabled);

/**
 * @brief Send SSID setting
 * @param ssid SSID string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_send_ssid(const char *ssid);

/**
 * @brief Send password setting
 * @param password Password string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_client_send_password(const char *password);

/**
 * @brief Check if BAP client is connected
 * @return true if connected (recent response received), false otherwise
 */
bool bap_client_is_connected(void);

/**
 * @brief Reset connection state
 * Used when connection is lost and needs to be re-established
 */
void bap_client_reset_connection_state(void);

#ifdef __cplusplus
}
#endif