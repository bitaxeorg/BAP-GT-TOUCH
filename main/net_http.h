#pragma once

#include "esp_err.h"
#include "esp_http_client.h"

/**
 * Perform an HTTP request, preferring IPv6 when the display has a global IPv6
 * address. If the IPv6 connection fails (no AAAA record, no route, refused or
 * timed out before connecting), the request is retried once over IPv4 and
 * IPv6 is skipped for five minutes.
 * Without a global IPv6 address this is a single request with the default
 * address order. The config's addr_type is overwritten.
 *
 * status_code receives the HTTP status of the last attempt (0 if none).
 */
esp_err_t net_http_perform(esp_http_client_config_t *config, int *status_code);
