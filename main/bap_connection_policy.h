#pragma once

#include <stdbool.h>
#include <stdint.h>

bool bap_connection_retry_due(bool subscriptions_sent,
                              uint32_t subscriptions_sent_at,
                              uint32_t last_response_at,
                              uint32_t now,
                              uint32_t timeout);
