#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BAP_LINE_BUFFER_CAPACITY 256U

typedef struct {
    char data[BAP_LINE_BUFFER_CAPACITY];
    size_t length;
    bool dropping_oversized_line;
} bap_line_buffer_t;

void bap_line_buffer_init(bap_line_buffer_t *buffer);
bool bap_line_buffer_push(bap_line_buffer_t *buffer, uint8_t byte,
                          const char **complete_line);
