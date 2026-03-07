#ifndef WIFI_H
#define WIFI_H

#include "lvgl.h"
#include "home.h"  // For screen dimensions and colors

typedef struct {
    char ssid[64];
    char password[128];
    char ip_address[16];
    char gateway[16];
    char subnet_mask[16];
    char dns[16];
    bool is_connected;
    int signal_strength; // -100 to 0 dBm
} wifi_info_t;

// Function declarations
void wifi_screen_create(void);
void wifi_screen_destroy(void);
void wifi_update_info(const wifi_info_t* info);
void wifi_update_ssid_list(const char* ssids[], int count);
void wifi_task_handler(void); 
lv_obj_t* wifi_get_screen(void);

// Individual update functions for BAP responses
void wifi_update_ssid(const char* ssid);
void wifi_update_rssi(const char* rssi);
void wifi_update_ip(const char* ip);
void wifi_update_password(const char* password);
bool wifi_is_connected(void);
const char *wifi_get_current_ip(void);

// Event handlers
void wifi_connect_clicked(lv_event_t * e);
void wifi_disconnect_clicked(lv_event_t * e);
void wifi_scan_clicked(lv_event_t * e);
void wifi_home_clicked(lv_event_t * e);
void wifi_block_clicked(lv_event_t * e);
void wifi_clock_clicked(lv_event_t * e);
void wifi_price_clicked(lv_event_t * e);
void wifi_mempool_clicked(lv_event_t * e);
void wifi_settings_clicked(lv_event_t * e);
void wifi_night_clicked(lv_event_t * e);

#endif // WIFI_H
