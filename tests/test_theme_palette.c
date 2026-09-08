#include <assert.h>
#include <stdio.h>

#include "theme_palette.h"

int main(void)
{
    assert(ACCENT_THEME_DEFAULT == ACCENT_THEME_RED);
    assert(!theme_palette_is_valid((accent_theme_t)-1));
    assert(!theme_palette_is_valid(ACCENT_THEME_COUNT));

    const uint32_t expected_accents[] = {
        0xD4021B,
        0xFF6700,
        0x1E84E5,
        0x2DC260,
        0xB146F5,
    };
    const uint16_t panel_verified[] = {0xd003, 0xfb20, 0x1c3c, 0x2e0c, 0xb23e};
    const uint32_t expected_text[] = {
        0x000000,
        0x000000,
        0xFFFFFF,
        0x000000,
        0xFFFFFF,
    };

    for (int i = 0; i < ACCENT_THEME_COUNT; ++i) {
        accent_theme_t theme = (accent_theme_t)i;
        assert(theme_palette_is_valid(theme));
        assert(theme_palette_accent_hex(theme) == expected_accents[i]);
        assert(theme_palette_text_hex(theme) == expected_text[i]);
        uint32_t rgb = theme_palette_accent_hex(theme);
        uint16_t rgb565 = ((rgb >> 8) & 0xf800) | ((rgb >> 5) & 0x07e0) | ((rgb >> 3) & 0x001f);
        assert(rgb565 == panel_verified[i]);
    }

    assert(theme_palette_accent_hex((accent_theme_t)99) == 0xD4021B);
    assert(theme_palette_text_hex((accent_theme_t)99) == 0x000000);
    puts("theme palette tests passed");
    return 0;
}
