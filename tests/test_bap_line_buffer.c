#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bap_line_buffer.h"

static const char *feed(bap_line_buffer_t *buffer, const char *text)
{
    const char *line = NULL;
    for (size_t i = 0; text[i] != '\0'; i++) {
        const char *candidate = NULL;
        if (bap_line_buffer_push(buffer, (uint8_t)text[i], &candidate)) {
            line = candidate;
        }
    }
    return line;
}

int main(void)
{
    bap_line_buffer_t buffer;
    bap_line_buffer_init(&buffer);

    assert(feed(&buffer, "$BAP,RES,hash") == NULL);
    const char *line = feed(&buffer, "rate,123*00\r\n");
    assert(line != NULL);
    assert(strcmp(line, "$BAP,RES,hashrate,123*00") == 0);

    assert(feed(&buffer, "$BAP,RES,power,40*00\n") != NULL);
    assert(feed(&buffer, "\n") == NULL);

    char oversized[BAP_LINE_BUFFER_CAPACITY + 16];
    memset(oversized, 'A', sizeof(oversized));
    oversized[sizeof(oversized) - 2] = '\n';
    oversized[sizeof(oversized) - 1] = '\0';
    assert(feed(&buffer, oversized) == NULL);

    line = feed(&buffer, "$BAP,RES,shares,10*00\r\n");
    assert(line != NULL);
    assert(strcmp(line, "$BAP,RES,shares,10*00") == 0);
    return 0;
}
