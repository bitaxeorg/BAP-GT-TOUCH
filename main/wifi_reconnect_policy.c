#include "wifi_reconnect_policy.h"

uint32_t wifi_reconnect_delay_ms(uint32_t attempt)
{
    static const uint32_t delays_ms[] = {1000, 2000, 5000, 10000, 30000};
    const uint32_t last = (uint32_t)(sizeof(delays_ms) / sizeof(delays_ms[0]) - 1);
    return delays_ms[attempt < last ? attempt : last];
}
