#ifndef THEME_PALETTE_H
#define THEME_PALETTE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ACCENT_THEME_RED = 0,
    ACCENT_THEME_BITCOIN_ORANGE,
    ACCENT_THEME_BLUE,
    ACCENT_THEME_GREEN,
    ACCENT_THEME_PURPLE,
    ACCENT_THEME_COUNT
} accent_theme_t;

#define ACCENT_THEME_DEFAULT ACCENT_THEME_RED

bool theme_palette_is_valid(accent_theme_t theme);
uint32_t theme_palette_accent_hex(accent_theme_t theme);
uint32_t theme_palette_text_hex(accent_theme_t theme);

#endif // THEME_PALETTE_H
