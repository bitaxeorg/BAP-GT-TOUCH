/**
 * @file bap_protocol.h
 * @brief BAP (Bitaxe accessory protocol) core protocol utilities
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAP_MAX_MESSAGE_LEN 256

typedef enum {
    BAP_CMD_SUB = 0,    // Subscribe
    BAP_CMD_RES,        // Response
    BAP_CMD_REQ,        // Request
    BAP_CMD_SET,        // Set
    BAP_CMD_CMD,        // Command
    BAP_CMD_UNKNOWN
} bap_command_t;

typedef struct {
    bap_command_t command;
    char command_str[16];
    char parameter[32];
    char value[64];
    char checksum_str[3];
    const char *raw_message;
} bap_message_t;

/**
 * @brief Convert BAP command enum to string
 * @param cmd Command enum
 * @return Command string or "UNK" if unknown
 */
const char* bap_command_to_string(bap_command_t cmd);

/**
 * @brief Convert command string to BAP command enum
 * @param cmd_str Command string
 * @return Command enum or BAP_CMD_UNKNOWN if not found
 */
bap_command_t bap_command_from_string(const char *cmd_str);

/**
 * @brief Calculate XOR checksum for sentence body
 * @param sentence_body The sentence body (without $ and *)
 * @return Calculated checksum
 */
uint8_t bap_calculate_checksum(const char *sentence_body);

/**
 * @brief Verify message checksum
 * @param message Full BAP message including checksum
 * @return true if checksum is valid, false otherwise
 */
bool bap_verify_checksum(const char *message);

/**
 * @brief Format a BAP message
 * @param output_buffer Buffer to store the formatted message
 * @param buffer_size Size of the output buffer
 * @param cmd Command type
 * @param parameter Parameter name
 * @param value Value (only used for SET commands, can be NULL for others)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_format_message(char *output_buffer, size_t buffer_size, 
                             bap_command_t cmd, const char *parameter, const char *value);

/**
 * @brief Format a frequency setting message
 * @param output_buffer Buffer to store the formatted message  
 * @param buffer_size Size of the output buffer
 * @param frequency_mhz Frequency in MHz
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_format_frequency_message(char *output_buffer, size_t buffer_size, float frequency_mhz);

/**
 * @brief Format a voltage setting message
 * @param output_buffer Buffer to store the formatted message
 * @param buffer_size Size of the output buffer  
 * @param voltage Voltage value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_format_voltage_message(char *output_buffer, size_t buffer_size, float voltage);

/**
 * @brief Format a fan speed setting message
 * @param output_buffer Buffer to store the formatted message
 * @param buffer_size Size of the output buffer  
 * @param speed_percent Fan speed percentage (0-100)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_format_fan_speed_message(char *output_buffer, size_t buffer_size, int speed_percent);

/**
 * @brief Parse BAP message header into structured format
 * @param message Raw BAP message
 * @param parsed Output structure for parsed message
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bap_parse_message_header(const char *message, bap_message_t *parsed);

#ifdef __cplusplus
}
#endif