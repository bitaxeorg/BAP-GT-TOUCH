#include "net_http.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "lwip/opt.h"

static const char *TAG = "net_http";

// After an IPv6 connection failure, use the default address order for a while
// so a broken IPv6 route doesn't delay every request.
#define IPV6_RETRY_INTERVAL_US (5 * 60 * 1000000LL)
static int64_t s_ipv6_retry_after_us;

// True when the station has a preferred global IPv6 address. Link-local and
// unique-local addresses can't reach public services, so they don't count.
static bool has_global_ipv6(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        return false;
    }
    esp_ip6_addr_t addrs[LWIP_IPV6_NUM_ADDRESSES];
    int count = esp_netif_get_all_preferred_ip6(netif, addrs);
    for (int i = 0; i < count; i++) {
        if (esp_netif_ip6_get_addr_type(&addrs[i]) == ESP_IP6_ADDR_IS_GLOBAL) {
            return true;
        }
    }
    return false;
}

esp_err_t net_http_perform(esp_http_client_config_t *config, int *status_code)
{
    esp_http_client_addr_type_t families[2] = {HTTP_ADDR_TYPE_UNSPEC, HTTP_ADDR_TYPE_UNSPEC};
    int count = 1;
    if (esp_timer_get_time() >= s_ipv6_retry_after_us && has_global_ipv6()) {
        families[0] = HTTP_ADDR_TYPE_INET6;
        families[1] = HTTP_ADDR_TYPE_INET;
        count = 2;
    }

    esp_err_t err = ESP_FAIL;
    *status_code = 0;
    for (int i = 0; i < count; i++) {
        config->addr_type = families[i];
        esp_http_client_handle_t client = esp_http_client_init(config);
        if (!client) {
            return ESP_FAIL;
        }
        err = esp_http_client_perform(client);
        *status_code = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);

        // Only a failure to connect is retried, so a request is never sent twice.
        if (err != ESP_ERR_HTTP_CONNECT || i + 1 == count) {
            break;
        }
        ESP_LOGW(TAG, "IPv6 connection to %s failed, trying IPv4", config->host ? config->host : config->url);
        s_ipv6_retry_after_us = esp_timer_get_time() + IPV6_RETRY_INTERVAL_US;
    }
    return err;
}
