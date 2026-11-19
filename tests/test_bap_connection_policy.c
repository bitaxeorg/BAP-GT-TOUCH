#include <assert.h>
#include <stdint.h>

#include "bap_connection_policy.h"

int main(void)
{
    const uint32_t timeout = 12000U;

    assert(!bap_connection_retry_due(false, 1000U, 0U, 50000U, timeout));
    assert(!bap_connection_retry_due(true, 1000U, 0U, 12999U, timeout));
    assert(bap_connection_retry_due(true, 1000U, 0U, 13000U, timeout));

    assert(!bap_connection_retry_due(true, 1000U, 9000U, 20999U, timeout));
    assert(bap_connection_retry_due(true, 1000U, 9000U, 21000U, timeout));

    assert(!bap_connection_retry_due(true, UINT32_MAX - 5000U, 0U,
                                     6998U, timeout));
    assert(bap_connection_retry_due(true, UINT32_MAX - 5000U, 0U,
                                    6999U, timeout));

    assert(!bap_connection_retry_due(true, 1000U, 0U, 50000U, 0U));
    return 0;
}
