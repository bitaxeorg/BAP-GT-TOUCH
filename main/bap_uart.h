/**
 * @file bap_uart.h
 * @brief BAP UART hardware abstraction layer
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize UART for BAP communication
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_uart_init(void);

/**
 * @brief Write data to UART
 * @param data Data to write
 * @param length Length of data to write
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_uart_write(const char *data, size_t length);

/**
 * @brief Read data from UART
 * @param buffer Buffer to store received data
 * @param buffer_size Size of the buffer
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes read, or -1 on error
 */
int bap_uart_read(uint8_t *buffer, size_t buffer_size, uint32_t timeout_ms);

/**
 * @brief Flush UART buffers
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_uart_flush(void);

/**
 * @brief Deinitialize UART
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_uart_deinit(void);

/**
 * @brief Get UART buffer size
 * @return Buffer size in bytes
 */
size_t bap_uart_get_buffer_size(void);

#ifdef __cplusplus
}
#endif