#include "theme_palette.h"

bool theme_palette_is_valid(accent_theme_t theme)
{
    return theme >= ACCENT_THEME_RED && theme < ACCENT_THEME_COUNT;
}

uint32_t theme_palette_accent_hex(accent_theme_t theme)
{
    /* Panel-verified values avoid RGB565 trails seen with nearby colors. */
    switch (theme) {
    case ACCENT_THEME_BITCOIN_ORANGE:
        return 0xFF6700;
    case ACCENT_THEME_BLUE:
        return 0x1E84E5;
    case ACCENT_THEME_GREEN:
        return 0x2DC260;
    case ACCENT_THEME_PURPLE:
        return 0xB146F5;
    case ACCENT_THEME_RED:
    default:
        return 0xD4021B;
    }
}

uint32_t theme_palette_text_hex(accent_theme_t theme)
{
    switch (theme) {
    case ACCENT_THEME_BLUE:
    case ACCENT_THEME_PURPLE:
        return 0xFFFFFF;
    case ACCENT_THEME_RED:
    case ACCENT_THEME_BITCOIN_ORANGE:
    case ACCENT_THEME_GREEN:
    default:
        return 0x000000;
    }
}
