#include "bap_connection_policy.h"

bool bap_connection_retry_due(bool subscriptions_sent,
                              uint32_t subscriptions_sent_at,
                              uint32_t last_response_at,
                              uint32_t now,
                              uint32_t timeout)
{
    if (!subscriptions_sent || subscriptions_sent_at == 0U || timeout == 0U) {
        return false;
    }

    const uint32_t reference = last_response_at == 0U
        ? subscriptions_sent_at
        : last_response_at;
    return (uint32_t)(now - reference) >= timeout;
}
