#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
header="$root/main/waveshare_rgb_lcd_port.h"
source_file="$root/main/waveshare_rgb_lcd_port.c"

grep -q '#include "driver/i2c_master.h"' "$header"
! grep -q '#include "driver/i2c.h"' "$header"
grep -Eq 'I2C_MASTER_FREQ_HZ[[:space:]]+100000' "$header"
grep -q 'i2c_new_master_bus(&i2c_conf, &touch_i2c_bus)' "$source_file"
grep -q 'tp_io_config.scl_speed_hz = I2C_MASTER_FREQ_HZ' "$source_file"
grep -q 'esp_lcd_new_panel_io_i2c(touch_i2c_bus' "$source_file"
! grep -q 'i2c_driver_install' "$source_file"
! grep -q 'i2c_param_config' "$source_file"

echo "GT911 I2C driver contract checks passed"
