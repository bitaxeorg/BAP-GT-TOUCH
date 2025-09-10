/**
 * @file bap.h
 * @brief BAP (Bitaxe accessory protocol) main interface
 * 
 * This is the main header that provides backwards compatibility
 * and includes all BAP modules
 */

#ifndef BAP_H_
#define BAP_H_

#include "bap_client.h"
#include "bap_protocol.h"
#include "bap_uart.h"
#include "bap_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAP_init() bap_client_init()
#define BAP_parse_message(msg) bap_parse_and_handle_message(msg)
#define BAP_calculate_checksum(body) bap_calculate_checksum(body)
#define BAP_verify_checksum(msg) bap_verify_checksum(msg)
#define BAP_command_from_string(str) bap_command_from_string(str)
#define BAP_command_to_string(cmd) bap_command_to_string(cmd)
#define BAP_send_frequency_setting(freq) bap_client_send_frequency_setting(freq)
#define BAP_send_fan_speed(speed) bap_client_send_fan_speed(speed)
#define BAP_send_automatic_fan_control(enabled) bap_client_send_automatic_fan_control(enabled)
#define BAP_send_asic_voltage(vol) bap_client_send_asic_voltage(vol)
#define BAP_send_ssid(ssid) bap_client_send_ssid(ssid)
#define BAP_send_password(password) bap_client_send_password(password)
#define BAP_reset_connection_state() bap_client_reset_connection_state()
#define BAP_is_connected() bap_client_is_connected()

static inline void BAP_send_message(bap_command_t cmd, const char *parameter, const char *value) {
    if (cmd == BAP_CMD_SUB) {
        bap_client_subscribe(parameter);
    } else if (cmd == BAP_CMD_REQ) {
        bap_client_request(parameter);
    }
    // SET commands should use specific functions like BAP_send_frequency_setting
}

static inline void BAP_request_system_info(void) {
    bap_client_request("systemInfo");
}

#ifdef __cplusplus
}
#endif

#endif /* BAP_H_ */