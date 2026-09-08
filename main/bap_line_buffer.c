#include "bap_line_buffer.h"

#include <string.h>

void bap_line_buffer_init(bap_line_buffer_t *buffer)
{
    if (buffer) {
        memset(buffer, 0, sizeof(*buffer));
    }
}

bool bap_line_buffer_push(bap_line_buffer_t *buffer, uint8_t byte,
                          const char **complete_line)
{
    if (!buffer || !complete_line) {
        return false;
    }
    *complete_line = NULL;

    if (byte == '\n') {
        if (buffer->dropping_oversized_line) {
            buffer->dropping_oversized_line = false;
            buffer->length = 0;
            return false;
        }
        if (buffer->length > 0 && buffer->data[buffer->length - 1] == '\r') {
            buffer->length--;
        }
        if (buffer->length == 0) {
            return false;
        }
        buffer->data[buffer->length] = '\0';
        *complete_line = buffer->data;
        buffer->length = 0;
        return true;
    }

    if (buffer->dropping_oversized_line) {
        return false;
    }
    if (buffer->length >= sizeof(buffer->data) - 1) {
        buffer->length = 0;
        buffer->dropping_oversized_line = true;
        return false;
    }

    buffer->data[buffer->length++] = (char)byte;
    return false;
}
