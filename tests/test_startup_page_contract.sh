#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
settings_file="$repo_dir/main/settings.c"
settings_header="$repo_dir/main/settings.h"
loading_file="$repo_dir/main/loading.c"

rg -q 'STARTUP_PAGE_HOME = 0' "$settings_header"
rg -q 'STARTUP_PAGE_COUNT' "$settings_header"
rg -Uq '"Home Dashboard\\n"[[:space:]]*"Latest Blocks\\n"[[:space:]]*"Mempool\\n"[[:space:]]*"Clock\\n"[[:space:]]*"Bitcoin Price\\n"[[:space:]]*"Hashrate Chart"' "$settings_file"
rg -Uq 'STARTUP_PAGE_HOME,[[:space:]]*STARTUP_PAGE_BLOCKS,[[:space:]]*STARTUP_PAGE_MEMPOOL,[[:space:]]*STARTUP_PAGE_CLOCK,[[:space:]]*STARTUP_PAGE_PRICE,[[:space:]]*STARTUP_PAGE_HASHRATE,' "$settings_file"
rg -q 'SETTINGS_NVS_STARTUP_PAGE_KEY "start_page"' "$settings_file"
rg -q 'nvs_set_u8\(handle, SETTINGS_NVS_STARTUP_PAGE_KEY' "$settings_file"
rg -q 'saved_startup_page < STARTUP_PAGE_COUNT' "$settings_file"
rg -q 'startup_page_dropdown_index\(current_startup_page\)' "$settings_file"
rg -q 'startup_page_option_values\[selected_index\]' "$settings_file"
rg -q '"Default Page:"' "$settings_file"

rg -q 'startup_page_t startup_page = settings_get_startup_page\(\)' "$loading_file"
rg -q 'Loading configured startup page: %u' "$loading_file"
rg -q 'switch \(startup_page\)' "$loading_file"
for page in PRICE BLOCKS MEMPOOL CLOCK HASHRATE HOME; do
    rg -q "case STARTUP_PAGE_${page}:" "$loading_file"
done
rg -q '^    default:' "$loading_file"
for screen in price block mempool clock night home; do
    rg -q "${screen}_screen_create\(\)" "$loading_file"
    rg -q "lv_scr_load\(${screen}_get_screen\(\)\)" "$loading_file"
done

echo "startup page contract tests passed"
