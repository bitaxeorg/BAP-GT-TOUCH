/**
 * @file bap_parser.c
 * @brief BAP message parsing and UI integration
 * 
 * Handles parsing of incoming BAP messages and updating the UI
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "bap_parser.h"
#include "bap_protocol.h"
#include "home.h"
#include "wifi.h"
#include "lvgl_port.h"

static const char *TAG = "BAP_PARSER";

esp_err_t bap_parse_and_handle_message(const char *message) {
    if (message == NULL) {
        ESP_LOGE(TAG, "Received NULL message");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received: %s", message);
    
    bap_message_t parsed_msg;
    esp_err_t ret = bap_parse_message_header(message, &parsed_msg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse message header");
        return ret;
    }
    
    if (parsed_msg.command == BAP_CMD_RES) {
        // Verify checksum for RES messages
        if (!bap_verify_checksum(message)) {
            uint8_t calculated = bap_calculate_checksum(parsed_msg.parameter);
            uint8_t received = (uint8_t)strtol(parsed_msg.checksum_str, NULL, 16);
            ESP_LOGE(TAG, "Checksum mismatch for %s: calculated %02X, received %02X",
                    parsed_msg.parameter, calculated, received);
            return ESP_ERR_INVALID_CRC;
        }
        return bap_handle_response(&parsed_msg);
    } else if (parsed_msg.command == BAP_CMD_CMD) {
        // Handle CMD messages (checksum already verified by protocol parser)
        if (!bap_verify_checksum(message)) {
            uint8_t calculated = bap_calculate_checksum(parsed_msg.parameter);
            uint8_t received = (uint8_t)strtol(parsed_msg.checksum_str, NULL, 16);
            ESP_LOGE(TAG, "Checksum mismatch for CMD %s: calculated %02X, received %02X",
                    parsed_msg.parameter, calculated, received);
            return ESP_ERR_INVALID_CRC;
        }
        return bap_handle_response(&parsed_msg);
    } else {
        ESP_LOGD(TAG, "Ignoring message type: %s", parsed_msg.command_str);
        return ESP_OK;
    }
}

esp_err_t bap_handle_response(const bap_message_t *msg) {
    if (!msg) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = ESP_OK;
    
    if (strcmp(msg->parameter, "hashrate") == 0) {
        ret = bap_handle_hashrate_response(msg->value);
    } else if (strcmp(msg->parameter, "chipTemp") == 0) {
        ret = bap_handle_temperature_response(msg->value);
    } else if (strcmp(msg->parameter, "power") == 0) {
        ret = bap_handle_power_response(msg->value);
    } else if (strcmp(msg->parameter, "shares") == 0) {
        ret = bap_handle_shares_response(msg->value);
    } else if (strcmp(msg->parameter, "deviceModel") == 0) {
        ret = bap_handle_device_model_response(msg->value);
    } else if (strcmp(msg->parameter, "asicModel") == 0) {
        ret = bap_handle_asic_model_response(msg->value);
    } else if (strcmp(msg->parameter, "pool") == 0) {
        ret = bap_handle_pool_url_response(msg->value);
    } else if (strcmp(msg->parameter, "poolPort") == 0) {
        ret = bap_handle_pool_port_response(msg->value);
    } else if (strcmp(msg->parameter, "poolUser") == 0) {
        ret = bap_handle_pool_user_response(msg->value);
    } else if (strcmp(msg->parameter, "fan_speed") == 0) {
        ret = bap_handle_fan_rpm_response(msg->value);
    } else if (strcmp(msg->parameter, "best_difficulty") == 0) {
        ret = bap_handle_best_difficulty_response(msg->value);
    } else if (strcmp(msg->parameter, "voltage") == 0) {
        ESP_LOGI(TAG, "Received voltage: %s", msg->value);
    } else if (strcmp(msg->parameter, "wifi_ssid") == 0) {
        ret = bap_handle_wifi_ssid_response(msg->value);
    } else if (strcmp(msg->parameter, "wifi_rssi") == 0) {
        ret = bap_handle_wifi_rssi_response(msg->value);
    } else if (strcmp(msg->parameter, "wifi_ip") == 0) {
        ret = bap_handle_wifi_ip_response(msg->value);
    } else if (strcmp(msg->parameter, "mode") == 0) {
        ret = bap_handle_mode(msg->value);
    } else {
        ESP_LOGI(TAG, "Received RES for %s: %s", msg->parameter, msg->value);
        // Unknown parameter, but not an error
    }
    
    return ret;
}

esp_err_t bap_handle_hashrate_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Valid hashrate: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_hashrate(value);
        
        // Also update night mode screen if it exists
        extern lv_obj_t* night_get_screen(void);
        if (night_get_screen()) {
            extern void night_update_hashrate(const char* hashrate);
            night_update_hashrate(value);
        }
        
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for hashrate update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_temperature_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    float temp_value = atof(value);
    char formatted_temp[16];
    snprintf(formatted_temp, sizeof(formatted_temp), "%.2f", temp_value);
    
    ESP_LOGI(TAG, "Valid chipTemp: %s", formatted_temp);
    
    if (lvgl_port_lock(100)) {
        home_update_temperature(formatted_temp);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for temperature update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_power_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received power: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_power(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for power update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_fan_rpm_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Received fan RPM: %s", value);

    if (lvgl_port_lock(100)) {
        home_update_fan_speed(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for fan speed update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_shares_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received shares: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_shares(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for shares update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_best_difficulty_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received best difficulty: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_best_difficulty(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for best difficulty update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_device_model_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received device model: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_device_model(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for device model update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_asic_model_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received ASIC model: %s", value);
    
    if (lvgl_port_lock(100)) {
        home_update_asic_model(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for ASIC model update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_pool_url_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received pool URL: %s", value);
    
    pool_info_t pool_update = {0};
    strncpy(pool_update.url, value, sizeof(pool_update.url) - 1);
    
    if (lvgl_port_lock(100)) {
        home_update_pool_info(&pool_update);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for pool info update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_pool_port_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received pool port: %s", value);
    
    pool_info_t pool_update = {0};
    strncpy(pool_update.port, value, sizeof(pool_update.port) - 1);
    
    if (lvgl_port_lock(100)) {
        home_update_pool_info(&pool_update);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for pool port update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_pool_user_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received pool user: %s", value);
    
    pool_info_t pool_update = {0};
    strncpy(pool_update.worker_name, value, sizeof(pool_update.worker_name) - 1);
    
    if (lvgl_port_lock(100)) {
        home_update_pool_info(&pool_update);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for pool user update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_wifi_ssid_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received WiFi SSID: %s", value);
    
    if (lvgl_port_lock(100)) {
        wifi_update_ssid(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for WiFi SSID update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_wifi_rssi_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received WiFi RSSI: %s", value);
    
    if (lvgl_port_lock(100)) {
        wifi_update_rssi(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for WiFi RSSI update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_wifi_ip_response(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received WiFi IP: %s", value);
    
    if (lvgl_port_lock(100)) {
        wifi_update_ip(value);
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for WiFi IP update");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t bap_handle_mode(const char *value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Received mode: %s", value);
    

    if (lvgl_port_lock(100)) {
        if(strcmp(value, "ap_mode") == 0) {
            home_wifi_clicked(NULL);
        }
        lvgl_port_unlock();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to acquire LVGL mutex for mode update");
        return ESP_ERR_TIMEOUT;
    }
}