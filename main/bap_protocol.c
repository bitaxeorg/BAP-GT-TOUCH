/**
 * @file bap_protocol.c
 * @brief BAP (Bitaxe accessory protocol) core protocol utilities
 * 
 * Handles message formatting, checksums, and protocol-level operations
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "bap_protocol.h"

static const char *TAG = "BAP_PROTOCOL";

static const char *command_strings[] = {
    "SUB",
    "RES", 
    "REQ",
    "SET",
    "CMD"
};

const char* bap_command_to_string(bap_command_t cmd) {
    if (cmd >= 0 && cmd < BAP_CMD_UNKNOWN) {
        return command_strings[cmd];
    }
    return "UNK";
}

bap_command_t bap_command_from_string(const char *cmd_str) {
    for (int i = 0; i < BAP_CMD_UNKNOWN; i++) {
        if (strcmp(cmd_str, command_strings[i]) == 0) {
            return (bap_command_t)i;
        }
    }
    return BAP_CMD_UNKNOWN;
}

uint8_t bap_calculate_checksum(const char *sentence_body) {
    uint8_t checksum = 0;
    while (*sentence_body) {
        checksum ^= (uint8_t)(*sentence_body++);
    }
    return checksum;
}

bool bap_verify_checksum(const char *message) {
    char cmd_str[16];
    char param_str[32];
    char value_str[64];
    char checksum_str[3];
    
    sscanf(message, "$BAP,%[^,],%[^,],%[^*]*%2s", cmd_str, param_str, value_str, checksum_str);
    
    // Extract the sentence body directly from the original message (between $ and *)
    char *asterisk_pos = strchr(message, '*');
    if (!asterisk_pos) {
        ESP_LOGE(TAG, "No asterisk found in message");
        return false;
    }
    
    // Calculate length of sentence body (from after $ to before *)
    size_t body_len = asterisk_pos - (message + 1);
    char sentence_body[BAP_MAX_MESSAGE_LEN];
    strncpy(sentence_body, message + 1, body_len);
    sentence_body[body_len] = '\0';
    
    uint8_t calculated_checksum = bap_calculate_checksum(sentence_body);
    uint8_t received_checksum = (uint8_t)strtol(checksum_str, NULL, 16);
    
    return calculated_checksum == received_checksum;
}

esp_err_t bap_format_message(char *output_buffer, size_t buffer_size, 
                             bap_command_t cmd, const char *parameter, const char *value) {
    if (!output_buffer || buffer_size == 0 || !parameter) {
        return ESP_ERR_INVALID_ARG;
    }

    char sentence_body[BAP_MAX_MESSAGE_LEN - 10]; // Reserve space for $, *, checksum, \r\n
    
    if (cmd == BAP_CMD_REQ) {
        int body_len = snprintf(sentence_body, sizeof(sentence_body), "BAP,%s,%s",
                               bap_command_to_string(cmd), parameter);
        
        if (body_len >= sizeof(sentence_body)) {
            ESP_LOGE(TAG, "Sentence body too long for REQ command");
            return ESP_ERR_INVALID_SIZE;
        }
        
        uint8_t checksum = bap_calculate_checksum(sentence_body);
        
        int msg_len = snprintf(output_buffer, buffer_size, "$%s*%02X\r\n", sentence_body, checksum);
        
        if (msg_len >= buffer_size) {
            ESP_LOGE(TAG, "Complete message too long for REQ command");
            return ESP_ERR_INVALID_SIZE;
        }
    } else if (cmd == BAP_CMD_SET && value) {
        int body_len = snprintf(sentence_body, sizeof(sentence_body), "BAP,SET,%s,%s",
                               parameter, value);
        
        if (body_len >= sizeof(sentence_body)) {
            ESP_LOGE(TAG, "Sentence body too long for SET command");
            return ESP_ERR_INVALID_SIZE;
        }
        
        uint8_t checksum = bap_calculate_checksum(sentence_body);
        
        int msg_len = snprintf(output_buffer, buffer_size, "$%s*%02X\r\n", sentence_body, checksum);
        
        if (msg_len >= buffer_size) {
            ESP_LOGE(TAG, "Complete message too long for SET command");
            return ESP_ERR_INVALID_SIZE;
        }
    } else {
        // SUB command (no checksum needed)
        int msg_len = snprintf(output_buffer, buffer_size, "$BAP,%s,%s\r\n",
                              bap_command_to_string(cmd), parameter);
        
        if (msg_len >= buffer_size) {
            ESP_LOGE(TAG, "Message too long for SUB command");
            return ESP_ERR_INVALID_SIZE;
        }
    }
    
    return ESP_OK;
}

esp_err_t bap_format_frequency_message(char *output_buffer, size_t buffer_size, float frequency_mhz) {
    char freq_str[32];
    snprintf(freq_str, sizeof(freq_str), "%.0f", frequency_mhz);
    
    return bap_format_message(output_buffer, buffer_size, BAP_CMD_SET, "frequency", freq_str);
}

esp_err_t bap_format_voltage_message(char *output_buffer, size_t buffer_size, float voltage) {
    char voltage_str[32];
    snprintf(voltage_str, sizeof(voltage_str), "%.2f", voltage);
    
    return bap_format_message(output_buffer, buffer_size, BAP_CMD_SET, "asic_voltage", voltage_str);
}

esp_err_t bap_format_fan_speed_message(char *output_buffer, size_t buffer_size, int speed_percent) {   
    char speed_str[32];
    snprintf(speed_str, sizeof(speed_str), "%d", speed_percent);
    
    return bap_format_message(output_buffer, buffer_size, BAP_CMD_SET, "fan_speed", speed_str);
}

esp_err_t bap_parse_message_header(const char *message, bap_message_t *parsed) {
    if (!message || !parsed) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(parsed, 0, sizeof(bap_message_t));
    
    // Format: $BAP,CMD,PARAM,VALUE*CS
    if (sscanf(message, "$BAP,%15[^,],%31[^,],%63[^*]*%2s", 
               parsed->command_str, parsed->parameter, parsed->value, parsed->checksum_str) != 4) {
        ESP_LOGE(TAG, "Failed to parse message header: %s", message);
        return ESP_ERR_INVALID_ARG;
    }
    
    parsed->command = bap_command_from_string(parsed->command_str);
    parsed->raw_message = message;
    
    return ESP_OK;
}