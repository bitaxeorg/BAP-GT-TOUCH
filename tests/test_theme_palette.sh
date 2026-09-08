#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_dir/main" \
    "$repo_dir/main/theme_palette.c" \
    "$repo_dir/tests/test_theme_palette.c" \
    -o "$tmp_dir/test_theme_palette"

"$tmp_dir/test_theme_palette"

home_header="$repo_dir/main/home.h"
settings_file="$repo_dir/main/settings.c"
lvgl_config="$repo_dir/components/lvgl__lvgl/lv_conf.h"
theme_file="$repo_dir/main/theme.c"
mempool_file="$repo_dir/main/mempool.c"
home_file="$repo_dir/main/home.c"

rg -Fq '#define COLOR_ACCENT        theme_get_accent_color()' "$home_header"
rg -Fq '#define COLOR_RED           lv_color_hex(0xD4021B)' "$home_header"
rg -Fq '#define THEME_NVS_ACCENT_KEY "accent"' "$theme_file"
rg -q 'theme_palette_is_valid.*saved_accent' "$theme_file"
rg -Uq '"Red\\n"[[:space:]]*"Orange\\n"[[:space:]]*"Blue\\n"[[:space:]]*"Green\\n"[[:space:]]*"Purple"' "$settings_file"
rg -Fq 'lv_async_call(settings_reload_theme_async, NULL)' "$settings_file"
rg -Fq 'lv_label_set_text(theme_title, "Display Color:")' "$settings_file"

nav_button_body="$(sed -n '/^static lv_obj_t \*create_nav_button(/,/^}/p' "$home_file")"
grep -Fq 'lv_obj_t *btn = lv_obj_create(parent)' <<<"$nav_button_body"
if grep -Fq 'lv_btn_create(parent)' <<<"$nav_button_body"; then
    echo "Home Hardware/Pool controls must not use the themed LVGL button class" >&2
    exit 1
fi
grep -Fq 'lv_obj_remove_style_all(btn)' <<<"$nav_button_body"
grep -Fq 'lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE)' <<<"$nav_button_body"
grep -Fq 'lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE)' <<<"$nav_button_body"
grep -Fq 'lv_obj_set_style_outline_width(btn, 0, 0)' <<<"$nav_button_body"
grep -Fq 'lv_obj_set_style_transform_width(btn, 0, 0)' <<<"$nav_button_body"
grep -Fq 'lv_obj_set_style_transform_height(btn, 0, 0)' <<<"$nav_button_body"

# Keep stock home button sizing while avoiding the LVGL button-class artifact.
rg -Fq '#define HOME_CARD_WIDTH (SCREEN_WIDTH - 60)' "$home_file"
rg -Fq '#define HOME_ACTION_ROW_WIDTH 680' "$home_file"
rg -Fq '#define HOME_ACTION_BUTTON_WIDTH 220' "$home_file"
rg -Fq 'lv_obj_set_size(main_cont, HOME_CARD_WIDTH, SCREEN_HEIGHT - 100)' "$home_file"
rg -Fq 'lv_obj_set_size(btn, HOME_ACTION_BUTTON_WIDTH, HOME_ACTION_BUTTON_HEIGHT)' "$home_file"
rg -Fq 'lv_obj_set_size(nav_cont, HOME_ACTION_ROW_WIDTH, 70)' "$home_file"
rg -Fq 'lv_obj_set_flex_align(nav_cont, LV_FLEX_ALIGN_SPACE_EVENLY' "$home_file"

# Mempool block visualization colors remain semantic and independent of the accent.
rg -Fq 'const lv_color_t color_height = lv_color_hex(0x00E5FF)' "$mempool_file"
rg -Fq 'const lv_color_t color_mid = lv_color_hex(0x1E5BFF)' "$mempool_file"
rg -Fq 'const lv_color_t color_bottom = lv_color_hex(0x7C3BFF)' "$mempool_file"
rg -Fq 'const lv_color_t color_fee = lv_color_hex(0xFFE600)' "$mempool_file"

# Partial-refresh buffers must not retain LVGL's larger pressed-state footprint.
rg -Fq '#define LV_THEME_DEFAULT_GROW 0' "$lvgl_config"
rg -Fq '#define LV_THEME_DEFAULT_TRANSITION_TIME 0' "$lvgl_config"

echo "theme source contract tests passed"
