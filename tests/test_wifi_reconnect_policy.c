#include "wifi_reconnect_policy.h"

#include <assert.h>

int main(void)
{
    assert(wifi_reconnect_delay_ms(0) == 1000);
    assert(wifi_reconnect_delay_ms(1) == 2000);
    assert(wifi_reconnect_delay_ms(2) == 5000);
    assert(wifi_reconnect_delay_ms(3) == 10000);
    assert(wifi_reconnect_delay_ms(4) == 30000);
    assert(wifi_reconnect_delay_ms(100) == 30000);
    return 0;
}
