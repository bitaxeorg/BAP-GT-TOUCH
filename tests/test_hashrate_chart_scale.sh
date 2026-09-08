#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_dir/main" \
    "$repo_dir/main/hashrate_chart_scale.c" \
    "$repo_dir/tests/test_hashrate_chart_scale.c" \
    -lm \
    -o "$test_dir/test_hashrate_chart_scale"

"$test_dir/test_hashrate_chart_scale"
