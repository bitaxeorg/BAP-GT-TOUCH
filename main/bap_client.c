/**
 * @file bap_client.c
 * @brief BAP client main logic, tasks, and connection management
 * 
 * Handles high-level BAP client operations, FreeRTOS tasks, and connection state
 */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "bap_client.h"
#include "bap_protocol.h"
#include "bap_uart.h"
#include "bap_parser.h"

static const char *TAG = "BAP_CLIENT";

static bool subscriptions_sent = false;
static bool system_info_requested = false;
static bool subscribed_hashrate = false;
static bool subscribed_temperature = false;
static bool subscribed_power = false;
static bool subscribed_fan_rpm = false;
static bool subscribed_shares = false;
static bool subscribed_best_difficulty = false;
static bool subscribed_wifi = false;
static uint32_t last_response_time = 0;

static void uart_send_task(void *pvParameters);
static void uart_receive_task(void *pvParameters);
static void connection_monitor_task(void *pvParameters);

esp_err_t bap_client_init(void) {
    ESP_LOGI(TAG, "BAP client initialization starting...");
    
    if (subscriptions_sent) {
        ESP_LOGW(TAG, "Subscriptions already sent, skipping initialization");
        return ESP_OK;
    }
    
    esp_err_t ret = bap_uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Creating UART tasks...");
    
    BaseType_t task_ret = xTaskCreate(uart_send_task, "uart_send", 4096, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART send task");
        return ESP_FAIL;
    }
    
    task_ret = xTaskCreate(uart_receive_task, "uart_receive", 4096, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART receive task");
        return ESP_FAIL;
    }
    
    task_ret = xTaskCreate(connection_monitor_task, "connection_monitor", 3072, NULL, 3, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create connection monitor task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BAP client initialization completed successfully");
    return ESP_OK;
}

esp_err_t bap_client_subscribe(const char *parameter) {
    if (!parameter) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_message(message, sizeof(message), BAP_CMD_SUB, parameter, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format subscription message for %s", parameter);
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send subscription for %s", parameter);
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_request(const char *parameter) {
    if (!parameter) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_message(message, sizeof(message), BAP_CMD_REQ, parameter, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format request message for %s", parameter);
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send request for %s", parameter);
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_frequency_setting(float frequency_mhz) {
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_frequency_message(message, sizeof(message), frequency_mhz);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format frequency message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send frequency setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent frequency setting: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_asic_voltage(float voltage) {
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_voltage_message(message, sizeof(message), voltage);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format voltage message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send voltage setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent ASIC voltage setting: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_fan_speed(int speed_percent) {
    if (speed_percent < 0 || speed_percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_fan_speed_message(message, sizeof(message), speed_percent);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format fan speed message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send fan speed setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent fan speed setting: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_automatic_fan_control(bool enabled) {
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_message(message, sizeof(message), BAP_CMD_SET, "auto_fan", enabled ? "1" : "0");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format auto fan control message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send auto fan control setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent auto fan control setting: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_ssid(const char *ssid) {
    if (!ssid || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_message(message, sizeof(message), BAP_CMD_SET, "ssid", ssid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format SSID message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send SSID setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent SSID setting: %s", message);
    return ESP_OK;
}

esp_err_t bap_client_send_password(const char *password) {
    if (!password || strlen(password) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char message[BAP_MAX_MESSAGE_LEN];
    esp_err_t ret = bap_format_message(message, sizeof(message), BAP_CMD_SET, "password", password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format password message");
        return ret;
    }
    
    ret = bap_uart_write(message, strlen(message));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send password setting");
        return ret;
    }
    
    ESP_LOGI(TAG, "Sent password setting: %s", message);
    return ESP_OK;
}

bool bap_client_is_connected(void) {
    if (last_response_time == 0) return false;
    uint32_t current_time = xTaskGetTickCount();
    return (current_time - last_response_time) < pdMS_TO_TICKS(30000);
}

void bap_client_reset_connection_state(void) {
    ESP_LOGI(TAG, "Resetting BAP connection state");
    subscriptions_sent = false;
    system_info_requested = false;
    subscribed_hashrate = false;
    subscribed_temperature = false;
    subscribed_power = false;
    subscribed_fan_rpm = false;
    subscribed_shares = false;
    subscribed_best_difficulty = false;
    subscribed_wifi = false;
    last_response_time = 0;
}

static esp_err_t bap_subscribe_hashrate(void) {
    if (subscribed_hashrate) {
        ESP_LOGW(TAG, "Already subscribed to hashrate, skipping");
        return ESP_OK;
    }
    
    esp_err_t ret = bap_client_subscribe("hashrate");
    if (ret == ESP_OK) {
        subscribed_hashrate = true;
        ESP_LOGI(TAG, "Subscribed to hashrate");
    }
    return ret;
}

static esp_err_t bap_subscribe_shares(void) {
    if(subscribed_shares) {
        ESP_LOGW(TAG, "Already subscribed to shares, skipping");
        return ESP_OK;
    }

    esp_err_t ret = bap_client_subscribe("shares");
    if (ret == ESP_OK) {
        subscribed_shares = true;
        ESP_LOGI(TAG, "Subscribed to shares");
    }
    return ret;
}

static esp_err_t bap_subscribe_wifi(void) {
    if (subscribed_wifi) {
        ESP_LOGW(TAG, "Already subscribed to WiFi, skipping");
        return ESP_OK;
    }

    esp_err_t ret = bap_client_subscribe("wifi");
    if (ret == ESP_OK) {
        subscribed_wifi = true;
        ESP_LOGI(TAG, "Subscribed to WiFi");
    }
    return ret;
}

static esp_err_t bap_subscribe_best_difficulty(void) {
    if (subscribed_best_difficulty) {
        ESP_LOGW(TAG, "Already subscribed to best difficulty, skipping");
        return ESP_OK;
    }
    esp_err_t ret = bap_client_subscribe("best_difficulty");
    if (ret == ESP_OK) {
        subscribed_best_difficulty = true;
        ESP_LOGI(TAG, "Subscribed to best difficulty");
    }
    return ret;
}

static esp_err_t bap_subscribe_temperature(void) {
    if (subscribed_temperature) {
        ESP_LOGW(TAG, "Already subscribed to temperature, skipping");
        return ESP_OK;
    }
    
    esp_err_t ret = bap_client_subscribe("temperature");
    if (ret == ESP_OK) {
        subscribed_temperature = true;
        ESP_LOGI(TAG, "Subscribed to temperature");
    }
    return ret;
}

static esp_err_t bap_subscribe_power(void) {
    if (subscribed_power) {
        ESP_LOGW(TAG, "Already subscribed to power, skipping");
        return ESP_OK;
    }
    
    esp_err_t ret = bap_client_subscribe("power");
    if (ret == ESP_OK) {
        subscribed_power = true;
        ESP_LOGI(TAG, "Subscribed to power");
    }
    return ret;
}

static esp_err_t bap_subscribe_fan_rpm(void) {
    if( subscribed_fan_rpm) {
        ESP_LOGW(TAG, "Already subscribed to fan RPM, skipping");
        return ESP_OK;
    }

    esp_err_t ret = bap_client_subscribe("fan_speed");
    if (ret == ESP_OK) {
        subscribed_fan_rpm = true;
        ESP_LOGI(TAG, "Subscribed to fan RPM");
    }
    return ret;
}

static esp_err_t bap_request_system_info(void) {
    if (system_info_requested) {
        ESP_LOGW(TAG, "System info already requested, skipping");
        return ESP_OK;
    }
    
    esp_err_t ret = bap_client_request("systemInfo");
    if (ret == ESP_OK) {
        system_info_requested = true;
        ESP_LOGI(TAG, "Sent system info request");
    }
    return ret;
}

static void uart_send_task(void *pvParameters) {
    if (subscriptions_sent) {
        ESP_LOGI(TAG, "Subscriptions already sent, exiting task");
        vTaskDelete(NULL);
        return;
    }
    
    // Wait 7 seconds before sending subscription
    ESP_LOGI(TAG, "Waiting 7 seconds before sending subscription...");
    vTaskDelay(pdMS_TO_TICKS(7000));
    
    bap_subscribe_hashrate();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_temperature();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_power();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_fan_rpm();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_shares();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_best_difficulty();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before next subscription
    bap_subscribe_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit
    bap_request_system_info();
    
    subscriptions_sent = true;
    ESP_LOGI(TAG, "All subscriptions sent, task exiting");
    vTaskDelete(NULL);
}

static void uart_receive_task(void *pvParameters) {
    static uint8_t buffer[1024]; // Use constant instead of bap_uart_get_buffer_size()
    
    esp_err_t wdt_ret = esp_task_wdt_add(NULL);
    if (wdt_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add UART receive task to watchdog: %s", esp_err_to_name(wdt_ret));
    }
    
    while (1) {
        if (wdt_ret == ESP_OK) {
            esp_task_wdt_reset();
        }
        
        int len = bap_uart_read(buffer, sizeof(buffer), 100);
        if (len > 0) {
            buffer[len] = '\0';
            
            char *message_start = (char*)buffer;
            char *message_end;
            
            while ((message_end = strstr(message_start, "\r\n")) != NULL) {
                *message_end = '\0';
                
                if (message_start[0] == '$') {
                    last_response_time = xTaskGetTickCount();
                    bap_parse_and_handle_message(message_start);
                }
                
                message_start = message_end + 2; // Skip \r\n
            }
            
            // Handle case where last message doesn't end with \r\n
            if (strlen(message_start) > 0 && message_start[0] == '$') {
                last_response_time = xTaskGetTickCount();
                bap_parse_and_handle_message(message_start);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void connection_monitor_task(void *pvParameters) {
    esp_err_t wdt_ret = esp_task_wdt_add(NULL);
    if (wdt_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add connection monitor task to watchdog: %s", esp_err_to_name(wdt_ret));
    }
    
    for (int i = 0; i < 150; i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (wdt_ret == ESP_OK) {
            esp_task_wdt_reset();
        }
    }
    
    while (1) {
        if (wdt_ret == ESP_OK) {
            esp_task_wdt_reset();
        }
        
        uint32_t current_time = xTaskGetTickCount();
        
        // Check if we haven't received any response for 12 seconds
        if (last_response_time > 0 && (current_time - last_response_time) > pdMS_TO_TICKS(12000)) {
            ESP_LOGW(TAG, "No response for 12s, resetting connection state");
            
            bap_client_reset_connection_state();
            
            BaseType_t task_ret = xTaskCreate(uart_send_task, "uart_send_retry", 4096, NULL, 5, NULL);
            if (task_ret != pdPASS) {
                ESP_LOGE(TAG, "Failed to create retry UART send task");
            } else {
                ESP_LOGI(TAG, "Created retry task to re-establish connection");
            }
        }
        
        // Check every 10 seconds, but reset watchdog more frequently
        for (int i = 0; i < 100; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (wdt_ret == ESP_OK) {
                esp_task_wdt_reset();
            }
        }
    }
}