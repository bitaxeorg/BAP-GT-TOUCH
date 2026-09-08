#!/usr/bin/env python3
"""Compile the production Wi-Fi state handlers against a deterministic ESP shim."""
from pathlib import Path
import subprocess
import tempfile
from test_network_data_contract import function_body

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'main/wifi.c').read_text()
names = {
    'wifi_set_connection_state': 'static void wifi_set_connection_state(wifi_connection_state_t state)',
    'wifi_schedule_reconnect': 'static void wifi_schedule_reconnect(void)',
    'wifi_begin_connection': 'static esp_err_t wifi_begin_connection(void)',
    'wifi_start_saved_connection': 'esp_err_t wifi_start_saved_connection(void)',
    'wifi_connect_with_credentials': 'static esp_err_t wifi_connect_with_credentials(const char *ssid, const char *password)',
    'wifi_is_connected': 'bool wifi_is_connected(void)',
    'wifi_task_handler': 'void wifi_task_handler(void)',
    'wifi_event_handler': 'static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)',
    'wifi_update_rssi': 'void wifi_update_rssi(const char *rssi)',
    'wifi_update_ip': 'void wifi_update_ip(const char *ip)',
}
shim = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wifi_reconnect_policy.h"
typedef int esp_err_t;
typedef int esp_event_base_t;
typedef unsigned EventBits_t;
typedef struct { struct { unsigned char ssid[32], password[64]; } sta; } wifi_config_t;
typedef struct { struct { unsigned addr; } ip; } esp_netif_ip_info_t;
typedef struct { int rssi; } wifi_ap_record_t;
typedef enum { WIFI_CONNECTION_STATE_DISCONNECTED, WIFI_CONNECTION_STATE_CONNECTING,
 WIFI_CONNECTION_STATE_CONNECTED, WIFI_CONNECTION_STATE_FAILED } wifi_connection_state_t;
#define ESP_OK 0
#define ESP_ERR_NOT_FOUND 1
#define ESP_ERR_INVALID_ARG 2
#define WIFI_IF_STA 0
#define WIFI_EVENT 1
#define IP_EVENT 2
#define WIFI_EVENT_SCAN_DONE 3
#define WIFI_EVENT_STA_DISCONNECTED 4
#define IP_EVENT_STA_GOT_IP 5
#define WIFI_LINK_CHANGED 1
#define pdTRUE 1
#define pdFALSE 0
#define IPSTR "%u"
#define IP2STR(ip) ((ip)->addr)
#define LV_UNUSED(x) (void)(x)
#define ESP_LOGW(...) ((void)0)
static bool wifi_connect_pending, wifi_reconnect_enabled, scan_event_received;
static uint32_t wifi_reconnect_attempt;
static int64_t wifi_reconnect_due_us, wifi_connect_deadline_us, now;
typedef int esp_netif_t;
static int wifi_link_events = 1, station;
static esp_netif_t *wifi_sta_netif = &station;
static wifi_connection_state_t wifi_connection_state;
static struct { bool is_connected; char ssid[64], ip_address[16]; int signal_strength; } current_wifi_info;
static char miner_ip[16];
static wifi_config_t saved;
static int connects, disconnects, configs, ui_updates, init_result, connect_result;
static bool link_up;
static unsigned address, events;
static int64_t esp_timer_get_time(void) { return now; }
static int wifi_init_common(void) { return init_result; }
static void wifi_check_scan_completion(void) {}
static void wifi_refresh_status_ui(void) { ui_updates++; }
static int esp_wifi_connect(void) { connects++; return connect_result; }
static int esp_wifi_disconnect(void) { disconnects++; link_up=false; events |= WIFI_LINK_CHANGED; return 0; }
static int esp_wifi_get_config(int iface, wifi_config_t *cfg) { (void)iface; *cfg=saved; return 0; }
static int esp_wifi_set_config(int iface, const wifi_config_t *cfg) { (void)iface; saved=*cfg; configs++; return 0; }
static esp_netif_t *esp_netif_get_handle_from_ifkey(const char *s) { (void)s; return wifi_sta_netif; }
static bool esp_netif_is_netif_up(esp_netif_t *n) { (void)n; return link_up; }
static int esp_netif_get_ip_info(const void *n, esp_netif_ip_info_t *ip) { (void)n; ip->ip.addr=address; return 0; }
static int esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap) { ap->rssi=-50; return 0; }
static EventBits_t xEventGroupWaitBits(int g, unsigned mask, int clear, int all, int delay) {
 (void)g; (void)clear; (void)all; (void)delay; unsigned e=events & mask; events &= ~mask; return e;
}
static void xEventGroupSetBits(int g, unsigned mask) { (void)g; events |= mask; }
'''
prototypes = '\n'.join(signature + ';' for signature in names.values())
functions = '\n'.join(signature+'{'+function_body(source, name)+'}' for name, signature in names.items())
checks = r'''
int main(void) {
 now=1000;
 assert(wifi_start_saved_connection()==ESP_ERR_NOT_FOUND && connects==0);
 strcpy((char*)saved.sta.ssid,"test-network");
 assert(wifi_start_saved_connection()==ESP_OK && connects==1 && wifi_connect_pending);
 int64_t deadline=wifi_connect_deadline_us;
 assert(wifi_start_saved_connection()==ESP_OK && connects==1);
 assert(wifi_connect_with_credentials("test-network","")==ESP_OK);
 assert(connects==1 && configs==0 && wifi_connect_deadline_us==deadline);
 wifi_update_rssi("-30"); wifi_update_ip("test-miner");
 assert(!current_wifi_info.is_connected && wifi_connect_pending);
 assert(strcmp(miner_ip,"test-miner")==0);
 address=42; link_up=false; assert(!wifi_is_connected());
 link_up=true; int before=ui_updates;
 wifi_event_handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);
 assert(ui_updates==before && wifi_connect_pending); // event task does not touch UI
 wifi_task_handler();
 assert(current_wifi_info.is_connected && !wifi_connect_pending && wifi_reconnect_attempt==0);
 assert(wifi_connect_with_credentials("test-network","")==ESP_OK && connects==1);
 link_up=false;
 wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
 wifi_task_handler();
 assert(wifi_reconnect_due_us==now+1000000 && connects==1);
 now=wifi_reconnect_due_us-1; wifi_task_handler(); assert(connects==1);
 now++; wifi_task_handler(); assert(connects==2 && wifi_connect_pending);
 now=wifi_connect_deadline_us; wifi_task_handler();
 assert(disconnects==1 && wifi_reconnect_due_us==now+2000000);
 wifi_task_handler(); assert(connects==2); // duplicate disconnect does not restart deadline
 now=wifi_reconnect_due_us; connect_result=9; wifi_task_handler();
 assert(connects==3 && wifi_reconnect_due_us==now+5000000);
 connect_result=0;
 assert(wifi_connect_with_credentials("replacement","")==ESP_OK && configs==1);
 wifi_task_handler();
 now=wifi_reconnect_due_us; wifi_task_handler(); assert(connects==4);
 assert(wifi_connect_with_credentials("replacement","")==ESP_OK && connects==4 && configs==1);
 assert(wifi_connect_with_credentials("","")==ESP_ERR_INVALID_ARG);
 puts("PASS: production startup, credential echo, local link, event-thread isolation, timeout and reconnect handlers");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp)/'runtime.c'; c.write_text(shim+prototypes+functions+checks)
    exe = Path(tmp)/'runtime'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I',str(ROOT/'main'),str(c),
                    str(ROOT/'main/wifi_reconnect_policy.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
