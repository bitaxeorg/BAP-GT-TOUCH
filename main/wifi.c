#include "wifi.h"
#include "home.h"
#include "bap.h"
#include "settings.h"
#include "night.h"
#include "block.h"
#include "clock.h"
#include "price.h"
#include "mempool.h"
#include "stdio.h"
#include "string.h"
#include "custom_fonts.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

static const char* TAG = "wifi_screen";

static wifi_ap_record_t *scan_results = NULL;
static uint16_t scan_count = 0;
static bool scan_in_progress = false;
static bool scan_completed = false;
static bool scan_event_received = false;

static lv_obj_t * wifi_screen = NULL;
static lv_obj_t * ssid_label = NULL;
static lv_obj_t * status_label = NULL;
static lv_obj_t * ip_label = NULL;
static lv_obj_t * signal_label = NULL;
static lv_obj_t * ssid_ta = NULL;
static lv_obj_t * ssid_dropdown = NULL;
static lv_obj_t * password_ta = NULL;
static lv_obj_t * keyboard = NULL;
static lv_obj_t * password_toggle_btn = NULL;
static bool wifi_initialized = false;
static bool wifi_event_handlers_registered = false;
static bool wifi_connect_pending = false;
static esp_netif_t *wifi_sta_netif = NULL;
static bool wifi_bap_ssid_received = false;
static bool wifi_bap_password_received = false;

static wifi_info_t current_wifi_info = {
    .ssid = "MyNetwork",
    .password = "",
    .ip_address = "192.168.1.100",
    .subnet_mask = "255.255.255.0",
    .dns = "8.8.8.8",
    .is_connected = false,
    .signal_strength = -45
};

static void wifi_try_connect_from_bap(void);
static esp_err_t wifi_init_common(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static lv_obj_t* create_wifi_button(lv_obj_t* parent, const char* text, lv_event_cb_t event_cb)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 190, 48);
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_TEXT_ON_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
    
    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

// Create a text area for input
static lv_obj_t* create_input_field(lv_obj_t* parent, const char* placeholder)
{
    lv_obj_t * ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 320, 44);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_style_bg_color(ta, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_border_color(ta, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(ta, LV_OPA_50, 0);
    lv_obj_set_style_radius(ta, 8, 0);
    lv_obj_set_style_text_color(ta, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, 0);
    
    return ta;
}

// Create a dropdown for SSID selection
static lv_obj_t* create_ssid_dropdown(lv_obj_t* parent)
{
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, 320, 44);
    lv_dropdown_set_options(dd, "Select Network");
    lv_obj_set_style_bg_color(dd, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dd, 1, 0);
    lv_obj_set_style_border_color(dd, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(dd, LV_OPA_50, 0);
    lv_obj_set_style_radius(dd, 8, 0);
    lv_obj_set_style_text_color(dd, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
    
    // Style the dropdown list
    lv_obj_t * list = lv_dropdown_get_list(dd);
    if(list) {
        lv_obj_set_width(list, lv_pct(100));
        lv_obj_set_height(list, 260);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_scroll_dir(list, LV_DIR_VER);
        lv_obj_set_style_bg_color(list, COLOR_CARD_BG, 0);
        lv_obj_set_style_border_color(list, COLOR_ACCENT, 0);
        lv_obj_set_style_text_color(list, COLOR_TEXT_PRIMARY, 0);
    }
    
    return dd;
}

// WiFi scan completion handler
static void wifi_scan_done_handler(void)
{
    if(!ssid_dropdown) return;
    
    ESP_LOGI(TAG, "WiFi scan completed");
    ESP_LOGI(TAG, "Found %d networks", scan_count);
    
    if(scan_count > 0) {
        // Allocate memory for scan results
        if(scan_results) {
            free(scan_results);
        }
        scan_results = malloc(sizeof(wifi_ap_record_t) * scan_count);
        
        if(scan_results) {
            // Get the actual scan results
            esp_wifi_scan_get_ap_records(&scan_count, scan_results);
            
            // Create array of SSID strings for dropdown
            const char* ssid_list[scan_count];
            for(int i = 0; i < scan_count; i++) {
                ssid_list[i] = (char*)scan_results[i].ssid;
            }
            
            // Update the dropdown
            wifi_update_ssid_list(ssid_list, scan_count);
        } else {
            ESP_LOGE(TAG, "Failed to allocate memory for scan results");
            lv_dropdown_set_options(ssid_dropdown, "Scan failed - memory error");
        }
    } else {
        lv_dropdown_set_options(ssid_dropdown, "No networks found");
    }
    
    // Reset scan flags
    scan_in_progress = false;
    scan_completed = false;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    LV_UNUSED(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE received");
        // Set flag for LVGL task to process - don't call handler from event context
        // (event task has small stack, LVGL operations must be on LVGL task)
        scan_event_received = true;
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (wifi_connect_pending) {
            esp_wifi_connect();
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        current_wifi_info.is_connected = false;
        if (status_label) {
            lv_label_set_text(status_label, "Disconnected");
            lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
        }
        if (wifi_connect_pending) {
            esp_wifi_connect();
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        current_wifi_info.is_connected = true;
        snprintf(current_wifi_info.ip_address, sizeof(current_wifi_info.ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        if (status_label) {
            lv_label_set_text(status_label, "Connected");
            lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
        }
        if (ip_label) {
            char ip_text[32];
            snprintf(ip_text, sizeof(ip_text), "IP: %s", current_wifi_info.ip_address);
            lv_label_set_text(ip_label, ip_text);
        }
        return;
    }
}

static esp_err_t wifi_init_common(void)
{
    if (wifi_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!wifi_sta_netif) {
        wifi_sta_netif = esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!wifi_event_handlers_registered) {
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
        wifi_event_handlers_registered = true;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi initialized successfully");
    return ESP_OK;
}

// Initialize WiFi for scanning (if not already initialized)
static esp_err_t wifi_init_for_scan(void)
{
    return wifi_init_common();
}

// Check scan completion (called from LVGL task)
static void wifi_check_scan_completion(void)
{
    // Process scan completion when event was received
    // This runs in LVGL task context where it's safe to update UI and allocate memory
    if (scan_event_received && scan_in_progress && !scan_completed) {
        scan_event_received = false;
        esp_wifi_scan_get_ap_num(&scan_count);
        scan_completed = true;
        wifi_scan_done_handler();
    }
}

// Create a bottom navigation button
static lv_obj_t* create_bottom_nav_btn(lv_obj_t* parent, const char* symbol, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 56, 46);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);
    
    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

static lv_obj_t* create_bottom_nav_btn_img(lv_obj_t* parent, const lv_img_dsc_t *img_dsc, lv_event_cb_t event_cb, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 56, 46);
    lv_obj_set_style_bg_color(btn, active ? COLOR_ACCENT : COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, active ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn, COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(btn, active ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t * img = lv_img_create(btn);
    lv_img_set_src(img, img_dsc);
    lv_obj_set_style_img_recolor(img, active ? COLOR_TEXT_ON_ACCENT : COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_center(img);

    if(event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

// Text area event handler for keyboard show/hide
static void ta_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        if (keyboard && lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)) {
            lv_keyboard_set_textarea(keyboard, ta);
            lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(keyboard); 
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        if (keyboard && !lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_t* new_focused = lv_group_get_focused(lv_obj_get_group(ta));
            bool kb_focused = false;
            if(new_focused){
                lv_obj_t* parent_kb = new_focused;
                while(parent_kb){
                    if(parent_kb == keyboard) {
                        kb_focused = true;
                        break;
                    }
                    parent_kb = lv_obj_get_parent(parent_kb);
                }
            }
            if(!kb_focused){
                 lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Keyboard event handler for ready/cancel
static void keyboard_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(lv_keyboard_get_textarea(kb), LV_STATE_FOCUSED); 
    }
}

static void password_toggle_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (password_ta) {
            bool is_password_mode = lv_textarea_get_password_mode(password_ta);
            lv_textarea_set_password_mode(password_ta, !is_password_mode);
            
            lv_obj_t * label = lv_obj_get_child(password_toggle_btn, 0);
            if (label) {
                lv_label_set_text(label, is_password_mode ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
            }
        }
    }
}

void wifi_screen_create(void)
{
    // Don't create if already exists
    if(wifi_screen != NULL) {
        return;
    }
    
    // Create the main screen
    wifi_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifi_screen, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(wifi_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(wifi_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(wifi_screen, LV_SCROLLBAR_MODE_OFF);
    
    // Create main container - leave space for bottom nav
    lv_obj_t * main_cont = lv_obj_create(wifi_screen);
    lv_obj_set_size(main_cont, SCREEN_WIDTH - 60, SCREEN_HEIGHT - 100);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(main_cont, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(main_cont, 1, 0);
    lv_obj_set_style_border_color(main_cont, COLOR_BORDER, 0);
    lv_obj_set_style_border_opa(main_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(main_cont, 14, 0);
    lv_obj_set_style_pad_all(main_cont, 16, 0);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    
    // Title
    lv_obj_t * title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "WIFI SETTINGS");
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 6);
    
    // WiFi status container
    lv_obj_t * status_cont = lv_obj_create(main_cont);
    lv_obj_set_size(status_cont, 680, 120);
    lv_obj_align(status_cont, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_opa(status_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_cont, 0, 0);
    lv_obj_set_style_pad_all(status_cont, 10, 0);
    lv_obj_clear_flag(status_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // Current WiFi status (left side)
    lv_obj_t * current_wifi_cont = lv_obj_create(status_cont);
    lv_obj_set_size(current_wifi_cont, 320, 100);
    lv_obj_align(current_wifi_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(current_wifi_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(current_wifi_cont, 0, 0);
    lv_obj_set_style_pad_all(current_wifi_cont, 0, 0);
    
    lv_obj_t * wifi_icon = lv_label_create(current_wifi_cont);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_LEFT, 0, 0);
    
    ssid_label = lv_label_create(current_wifi_cont);
    lv_label_set_text(ssid_label, current_wifi_info.ssid);
    lv_obj_set_style_text_color(ssid_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_12, 0);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 40, 0);
    
    status_label = lv_label_create(current_wifi_cont);
    lv_label_set_text(status_label, current_wifi_info.is_connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 40, 25);
    
    signal_label = lv_label_create(current_wifi_cont);
    char signal_text[32];
    snprintf(signal_text, sizeof(signal_text), "Signal: %d dBm", current_wifi_info.signal_strength);
    lv_label_set_text(signal_label, signal_text);
    lv_obj_set_style_text_color(signal_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(signal_label, &lv_font_montserrat_12, 0);
    lv_obj_align(signal_label, LV_ALIGN_TOP_LEFT, 40, 45);
    
    // IP Address info (right side)
    lv_obj_t * ip_cont = lv_obj_create(status_cont);
    lv_obj_set_size(ip_cont, 320, 100);
    lv_obj_align(ip_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(ip_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ip_cont, 0, 0);
    lv_obj_set_style_pad_all(ip_cont, 0, 0);
    
    lv_obj_t * ip_icon = lv_label_create(ip_cont);
    lv_label_set_text(ip_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(ip_icon, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(ip_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(ip_icon, LV_ALIGN_TOP_LEFT, 0, 0);
    
    ip_label = lv_label_create(ip_cont);
    char ip_text[32];
    snprintf(ip_text, sizeof(ip_text), "IP: %s", current_wifi_info.ip_address);
    lv_label_set_text(ip_label, ip_text);
    lv_obj_set_style_text_color(ip_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 30, 0);
    
    // WiFi configuration container
    lv_obj_t * config_cont = lv_obj_create(main_cont);
    lv_obj_set_size(config_cont, 680, 130);
    lv_obj_align(config_cont, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_opa(config_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(config_cont, 0, 0);
    lv_obj_set_style_pad_all(config_cont, 10, 0);
    lv_obj_clear_flag(config_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // SSID dropdown
    lv_obj_t * ssid_label_input = lv_label_create(config_cont);
    lv_label_set_text(ssid_label_input, "Network Name (SSID):");
    lv_obj_set_style_text_color(ssid_label_input, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(ssid_label_input, &lv_font_montserrat_14, 0);
    lv_obj_align(ssid_label_input, LV_ALIGN_TOP_LEFT, 0, 0);
    
    ssid_dropdown = create_ssid_dropdown(config_cont);
    lv_obj_align(ssid_dropdown, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // Password input
    lv_obj_t * password_label_input = lv_label_create(config_cont);
    lv_label_set_text(password_label_input, "Password:");
    lv_obj_set_style_text_color(password_label_input, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(password_label_input, &lv_font_montserrat_14, 0);
    lv_obj_align(password_label_input, LV_ALIGN_TOP_LEFT, 350, 0);
    
    password_ta = create_input_field(config_cont, "Enter password");
    lv_obj_align(password_ta, LV_ALIGN_TOP_LEFT, 350, 25);
    lv_textarea_set_password_mode(password_ta, true);
    if (strlen(current_wifi_info.password) > 0) {
        lv_textarea_set_text(password_ta, current_wifi_info.password);
    }
    lv_obj_add_event_cb(password_ta, ta_event_handler, LV_EVENT_ALL, NULL);
    
    //password toggle button - positioned inside the password field
    password_toggle_btn = lv_btn_create(config_cont);
    lv_obj_set_size(password_toggle_btn, 30, 30);
    lv_obj_align(password_toggle_btn, LV_ALIGN_TOP_LEFT, 615, 30);
    lv_obj_set_style_bg_opa(password_toggle_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(password_toggle_btn, 0, 0);
    lv_obj_set_style_shadow_width(password_toggle_btn, 0, 0);
    lv_obj_set_style_radius(password_toggle_btn, 4, 0);
    
    lv_obj_set_style_bg_color(password_toggle_btn, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(password_toggle_btn, LV_OPA_10, LV_STATE_PRESSED);
    
    lv_obj_t * toggle_label = lv_label_create(password_toggle_btn);
    lv_label_set_text(toggle_label, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(toggle_label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(toggle_label, &lv_font_montserrat_14, 0);
    lv_obj_center(toggle_label);
    
    lv_obj_add_event_cb(password_toggle_btn, password_toggle_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Control buttons container
    lv_obj_t * control_cont = lv_obj_create(main_cont);
    lv_obj_set_size(control_cont, 680, 80);
    lv_obj_align(control_cont, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_set_style_bg_opa(control_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(control_cont, 0, 0);
    lv_obj_set_style_pad_all(control_cont, 0, 0);
    lv_obj_clear_flag(control_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(control_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Control buttons
    create_wifi_button(control_cont, "Connect", wifi_connect_clicked);
    create_wifi_button(control_cont, "Scan Networks", wifi_scan_clicked);
    
    // Create keyboard (hidden by default)
    keyboard = lv_keyboard_create(wifi_screen);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
    
    // Bottom navigation bar - full width at bottom of screen
    lv_obj_t * bottom_nav = lv_obj_create(wifi_screen);
    lv_obj_set_size(bottom_nav, SCREEN_WIDTH, 64);
    lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_nav, COLOR_NAV_BG, 0);
    lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_nav, 0, 0);
    lv_obj_set_style_radius(bottom_nav, 0, 0);
    lv_obj_set_style_pad_all(bottom_nav, 8, 0);
    lv_obj_clear_flag(bottom_nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(bottom_nav, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Bottom navigation buttons
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_HOME, wifi_home_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_STOP, wifi_block_clicked, false);
    create_bottom_nav_btn_img(bottom_nav, &cube_solid_full, wifi_mempool_clicked, false);
    create_bottom_nav_btn_img(bottom_nav, &clock_solid_full, wifi_clock_clicked, false);
    create_bottom_nav_btn(bottom_nav, "$", wifi_price_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_WIFI, NULL, true);  // WiFi is active
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_SETTINGS, wifi_settings_clicked, false);
    create_bottom_nav_btn(bottom_nav, LV_SYMBOL_EYE_OPEN, wifi_night_clicked, false);
}

void wifi_screen_destroy(void)
{
    if(wifi_screen) {
        // Clean up scan results
        if(scan_results) {
            free(scan_results);
            scan_results = NULL;
        }
        scan_in_progress = false;
        scan_completed = false;
        scan_event_received = false;

        lv_obj_del(wifi_screen);
        wifi_screen = NULL;
        ssid_label = NULL;
        status_label = NULL;
        ip_label = NULL;
        signal_label = NULL;
        ssid_ta = NULL;
        ssid_dropdown = NULL;
        password_ta = NULL;
        password_toggle_btn = NULL;
        keyboard = NULL;
    }
}

void wifi_update_info(const wifi_info_t* info)
{
    if(info) {
        memcpy(&current_wifi_info, info, sizeof(wifi_info_t));
        
        // Update display elements if they exist
        if(ssid_label) {
            lv_label_set_text(ssid_label, current_wifi_info.ssid);
        }
        
        if(status_label) {
            lv_label_set_text(status_label, current_wifi_info.is_connected ? "Connected" : "Disconnected");
            lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
        }
        
        if(ip_label) {
            char ip_text[32];
            snprintf(ip_text, sizeof(ip_text), "IP: %s", current_wifi_info.ip_address);
            lv_label_set_text(ip_label, ip_text);
        }
        
        if(signal_label) {
            char signal_text[32];
            snprintf(signal_text, sizeof(signal_text), "Signal: %d dBm", current_wifi_info.signal_strength);
            lv_label_set_text(signal_label, signal_text);
        }
    }
}

void wifi_update_ssid(const char* ssid)
{
    if(ssid) {
        strncpy(current_wifi_info.ssid, ssid, sizeof(current_wifi_info.ssid) - 1);
        current_wifi_info.ssid[sizeof(current_wifi_info.ssid) - 1] = '\0';
        wifi_bap_ssid_received = true;
        
        // Update display elements if they exist
        if(ssid_label) {
            lv_label_set_text(ssid_label, current_wifi_info.ssid);
        }
        wifi_try_connect_from_bap();
    }
}

void wifi_update_rssi(const char* rssi)
{
    if(rssi) {
        current_wifi_info.signal_strength = atoi(rssi);

        if(current_wifi_info.signal_strength > -128 && status_label) {
            current_wifi_info.is_connected = true; // Assume connected if we have a valid RSSI
            lv_label_set_text(status_label, "Connected");
            lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
        } else {
            current_wifi_info.is_connected = false; // Invalid RSSI indicates not connected
        }
        
        // Update display elements if they exist
        if(signal_label) {
            char signal_text[32];
            snprintf(signal_text, sizeof(signal_text), "Signal: %d dBm", current_wifi_info.signal_strength);
            lv_label_set_text(signal_label, signal_text);
        }
    }
}

void wifi_update_ip(const char* ip)
{
    if(ip) {
        strncpy(current_wifi_info.ip_address, ip, sizeof(current_wifi_info.ip_address) - 1);
        current_wifi_info.ip_address[sizeof(current_wifi_info.ip_address) - 1] = '\0';
        
        // Update display elements if they exist
        if(ip_label) {
            char ip_text[32];
            snprintf(ip_text, sizeof(ip_text), "IP: %s", current_wifi_info.ip_address);
            lv_label_set_text(ip_label, ip_text);
        }
    }
}

void wifi_update_password(const char* password)
{
    if (!password) {
        return;
    }

    strncpy(current_wifi_info.password, password, sizeof(current_wifi_info.password) - 1);
    current_wifi_info.password[sizeof(current_wifi_info.password) - 1] = '\0';
    wifi_bap_password_received = true;

    if (password_ta) {
        lv_textarea_set_text(password_ta, current_wifi_info.password);
    }

    wifi_try_connect_from_bap();
}

bool wifi_is_connected(void)
{
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta)
    {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(sta, &ip_info) == ESP_OK && ip_info.ip.addr != 0)
        {
            current_wifi_info.is_connected = true;
            return true;
        }
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        current_wifi_info.is_connected = true;
        return true;
    }

    current_wifi_info.is_connected = false;
    return false;
}

lv_obj_t* wifi_get_screen(void)
{
    return wifi_screen;
}

// Task handler - call this periodically from main LVGL task
void wifi_task_handler(void)
{
    wifi_check_scan_completion();
}

// Function to update the SSID dropdown with scan results
void wifi_update_ssid_list(const char* ssids[], int count)
{
    if(!ssid_dropdown) return;
    
    if(count <= 0) {
        lv_dropdown_set_options(ssid_dropdown, "No networks found");
        return;
    }
    
    // Build options string
    char options[2048] = {0}; // Buffer for all SSID options
    
    for(int i = 0; i < count && i < 20; i++) { // Limit to 20 networks
        if(i > 0) {
            strcat(options, "\n");
        }
        strncat(options, ssids[i], 63); // Limit SSID length
    }
    
    lv_dropdown_set_options(ssid_dropdown, options);
}

void wifi_connect_clicked(lv_event_t * e)
{
    // Get SSID from dropdown and password from text area
    if(ssid_dropdown && password_ta) {
        char selected_ssid[64] = {0};
        lv_dropdown_get_selected_str(ssid_dropdown, selected_ssid, sizeof(selected_ssid));
        const char* password = lv_textarea_get_text(password_ta);
        
        // Check if a valid network is selected
        if(strcmp(selected_ssid, "Select Network") == 0 || 
           strcmp(selected_ssid, "No networks found") == 0 ||
           strcmp(selected_ssid, "Scanning...") == 0) {
            printf("Please select a valid network first\n");
            return;
        }
        
        // Update current info
        //strncpy(current_wifi_info.ssid, selected_ssid, sizeof(current_wifi_info.ssid) - 1);
        //current_wifi_info.ssid[sizeof(current_wifi_info.ssid) - 1] = '\0';
        //
        //strncpy(current_wifi_info.password, password, sizeof(current_wifi_info.password) - 1);
        //current_wifi_info.password[sizeof(current_wifi_info.password) - 1] = '\0';
        
        BAP_send_ssid(selected_ssid);
        BAP_send_password(password);
    }
}

static void wifi_try_connect_from_bap(void)
{
    if (!wifi_bap_ssid_received || !wifi_bap_password_received) {
        return;
    }

    if (current_wifi_info.ssid[0] == '\0') {
        return;
    }

    esp_err_t ret = wifi_init_common();
    if (ret != ESP_OK) {
        return;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, current_wifi_info.ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, current_wifi_info.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return;
    }

    wifi_connect_pending = true;
    esp_wifi_connect();

    if (status_label) {
        lv_label_set_text(status_label, "Connecting...");
        lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
    }
}

void wifi_scan_clicked(lv_event_t * e)
{
    if(!ssid_dropdown) return;
    
    if(scan_in_progress) {
        ESP_LOGW(TAG, "Scan already in progress");
        return;
    }
    
    // Show scanning status
    lv_dropdown_set_options(ssid_dropdown, "Scanning...");
    
    // Initialize WiFi if needed
    esp_err_t ret = wifi_init_for_scan();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        lv_dropdown_set_options(ssid_dropdown, "WiFi init failed");
        return;
    }
    
    // Configure scan parameters
    wifi_scan_config_t scan_config = {
        .ssid = NULL,           // Scan all SSIDs
        .bssid = NULL,          // Scan all BSSIDs
        .channel = 0,           // Scan all channels
        .show_hidden = false,   // Don't show hidden networks
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,     // Minimum scan time per channel (ms)
                .max = 300      // Maximum scan time per channel (ms)
            }
        }
    };
    
    scan_in_progress = true;
    scan_completed = false;
    scan_event_received = false;

    // Start the WiFi scan (asynchronous)
    ret = esp_wifi_scan_start(&scan_config, false);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        lv_dropdown_set_options(ssid_dropdown, "Scan start failed");
        scan_in_progress = false;
        return;
    }
    
    ESP_LOGI(TAG, "WiFi scan started...");
}

void wifi_home_clicked(lv_event_t * e)
{
    home_screen_create();
    lv_scr_load(home_get_screen());
    wifi_screen_destroy();
}

void wifi_block_clicked(lv_event_t * e)
{
    block_screen_create();
    lv_scr_load(block_get_screen());
    wifi_screen_destroy();
}

void wifi_mempool_clicked(lv_event_t * e)
{
    mempool_screen_create();
    lv_scr_load(mempool_get_screen());
    wifi_screen_destroy();
}

void wifi_clock_clicked(lv_event_t * e)
{
    clock_screen_create();
    lv_scr_load(clock_get_screen());
    wifi_screen_destroy();
}

void wifi_price_clicked(lv_event_t * e)
{
    price_screen_create();
    lv_scr_load(price_get_screen());
    wifi_screen_destroy();
}

void wifi_settings_clicked(lv_event_t * e)
{
    settings_screen_create();
    lv_scr_load(settings_get_screen());
    wifi_screen_destroy();
}

void wifi_night_clicked(lv_event_t * e)
{
    // Navigate to night mode screen
    night_screen_create();
    lv_scr_load(night_get_screen());
    wifi_screen_destroy();
}
