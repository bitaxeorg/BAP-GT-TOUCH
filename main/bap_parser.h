/**
 * @file bap_parser.h
 * @brief BAP message parsing and UI integration
 */

#pragma once

#include "esp_err.h"
#include "bap_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse and handle a BAP message
 * @param message Raw BAP message string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_parse_and_handle_message(const char *message);

/**
 * @brief Handle a parsed BAP response message
 * @param msg Parsed BAP message structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_response(const bap_message_t *msg);

/**
 * @brief Handle hashrate response
 * @param value Hashrate value string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_hashrate_response(const char *value);

/**
 * @brief Handle temperature response
 * @param value Temperature value string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_temperature_response(const char *value);

/**
 * @brief Handle power response
 * @param value Power value string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_power_response(const char *value);

/**
 * @brief Handle fan RPM response
 * @param value Fan RPM value string
 * @return ESP_OK on success, error code otherwise
 
 */
esp_err_t bap_handle_fan_rpm_response(const char *value);

/**
 * @brief Handle share response
 * @param value Share value string
 * @return ESP_OK on success, error code otherwise
 * 
 */
 esp_err_t bap_handle_shares_response(const char *value);

/**
 * @brief Handle best difficulty response
 * @param value Best difficulty value string
 * @return ESP_OK on success, error code otherwise
 * 
 */
esp_err_t bap_handle_best_difficulty_response(const char *value);

/**
 * @brief Handle device model response
 * @param value Device model string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_device_model_response(const char *value);

/**
 * @brief Handle ASIC model response
 * @param value ASIC model string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_asic_model_response(const char *value);

/**
 * @brief Handle pool URL response
 * @param value Pool URL string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_pool_url_response(const char *value);

/**
 * @brief Handle pool port response
 * @param value Pool port string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_pool_port_response(const char *value);

/**
 * @brief Handle pool user response
 * @param value Pool user string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_pool_user_response(const char *value);

/**
 * @brief Handle WiFi SSID response
 * @param value WiFi SSID string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_wifi_ssid_response(const char *value);

/**
 * @brief Handle WiFi RSSI response
 * @param value WiFi RSSI string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_wifi_rssi_response(const char *value);

/**
 * @brief Handle WiFi IP response
 * @param value WiFi IP address string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_wifi_ip_response(const char *value);

/**
 * @brief Handle mode response
 * @param value Mode string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_handle_mode(const char *value);

#ifdef __cplusplus
}
#endif