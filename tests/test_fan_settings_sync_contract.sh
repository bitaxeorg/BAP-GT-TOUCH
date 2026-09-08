#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
parser_file="$repo_dir/main/bap_parser.c"
settings_file="$repo_dir/main/settings.c"
client_file="$repo_dir/main/bap_client.c"

rg -Fq 'strcmp(msg->parameter, "auto_fan") == 0' "$parser_file"
rg -Fq 'strcmp(msg->parameter, "manual_fan_speed") == 0' "$parser_file"
rg -Fq 'settings_update_auto_fan_control(strcmp(value, "1") == 0)' "$parser_file"
rg -Fq 'speed_percent < 0 || speed_percent > 100' "$parser_file"
rg -Fq 'settings_update_fan_speed_percent((int)speed_percent)' "$parser_file"
rg -Fq 'current_settings.auto_fan_control = enabled' "$settings_file"
rg -Fq 'current_settings.fan_speed_percent = speed_percent' "$settings_file"

# Mixed-version compatibility: old miners omit the two additive systemInfo
# responses. The display must keep using the legacy systemInfo request and fan
# RPM subscription, must not require unsupported dedicated subscriptions, and
# must leave its existing fan controls available for explicit user changes.
rg -Fq 'bap_client_request("systemInfo")' "$client_file"
rg -Fq 'bap_client_subscribe("fan_speed")' "$client_file"
rg -Fq 'bap_client_send_automatic_fan_control(bool enabled)' "$client_file"
rg -Fq 'bap_client_send_fan_speed(int speed_percent)' "$client_file"
if rg -q 'bap_client_(subscribe|request)\("(auto_fan|manual_fan_speed)"' "$client_file"; then
    echo "fan-state sync fields must remain optional for older ESP-Miner releases" >&2
    exit 1
fi

# Do not couple either optional response to the other. A partial/newer miner
# payload should still apply whichever valid setting it provides.
auto_handler="$(sed -n '/^esp_err_t bap_handle_auto_fan_response/,/^}/p' "$parser_file")"
manual_handler="$(sed -n '/^esp_err_t bap_handle_manual_fan_speed_response/,/^}/p' "$parser_file")"
if grep -q 'manual_fan_speed' <<<"$auto_handler" || grep -q 'auto_fan' <<<"$manual_handler"; then
    echo "optional fan-state responses must be handled independently" >&2
    exit 1
fi

echo "fan settings sync contract tests passed"
