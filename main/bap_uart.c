/**
 * @file bap_uart.c
 * @brief BAP UART hardware abstraction layer
 * 
 * Handles low-level UART initialization and communication
 */

#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bap_uart.h"

static const char *TAG = "BAP_UART";

#define BAP_UART_NUM UART_NUM_2
#define BAP_BUF_SIZE 1024
#define GPIO_BAP_RX 6
#define GPIO_BAP_TX 16

esp_err_t bap_uart_init(void) {
    ESP_LOGI(TAG, "Starting UART initialization...");
    
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_LOGI(TAG, "Installing UART driver...");
    esp_err_t ret = uart_driver_install(BAP_UART_NUM, BAP_BUF_SIZE * 2, BAP_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuring UART parameters...");
    ret = uart_param_config(BAP_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Setting UART pins individually (TX:%d, RX:%d)...", GPIO_BAP_TX, GPIO_BAP_RX);
    
    ESP_LOGI(TAG, "Setting TX pin %d...", GPIO_BAP_TX);
    ret = uart_set_pin(BAP_UART_NUM, GPIO_BAP_TX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART TX pin set failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Setting RX pin %d...", GPIO_BAP_RX);
    ret = uart_set_pin(BAP_UART_NUM, UART_PIN_NO_CHANGE, GPIO_BAP_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART RX pin set failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "UART initialization completed successfully");
    return ESP_OK;
}

esp_err_t bap_uart_write(const char *data, size_t length) {
    if (!data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int bytes_written = uart_write_bytes(BAP_UART_NUM, data, length);
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return ESP_FAIL;
    }
    
    if (bytes_written != length) {
        ESP_LOGW(TAG, "UART write incomplete: %d/%d bytes", bytes_written, length);
        return ESP_ERR_INVALID_SIZE;
    }
    
    return ESP_OK;
}

int bap_uart_read(uint8_t *buffer, size_t buffer_size, uint32_t timeout_ms) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    return uart_read_bytes(BAP_UART_NUM, buffer, buffer_size, timeout_ms / portTICK_PERIOD_MS);
}

esp_err_t bap_uart_flush(void) {
    return uart_flush(BAP_UART_NUM);
}

esp_err_t bap_uart_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing UART...");
    return uart_driver_delete(BAP_UART_NUM);
}

size_t bap_uart_get_buffer_size(void) {
    return BAP_BUF_SIZE;
}