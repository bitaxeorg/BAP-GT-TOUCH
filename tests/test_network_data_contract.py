#!/usr/bin/env python3
"""Source contracts for reliable network-backed data loading."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def source(name: str) -> str:
    return (ROOT / "main" / name).read_text(encoding="utf-8")


def function_body(text: str, name: str) -> str:
    match = re.search(
        rf"^[A-Za-z_][\w\s*]*\b{name}\s*\([^;{{}}]*?\)\s*\{{", text, re.M
    )
    if not match:
        raise AssertionError(f"function {name} not found")
    depth = 1
    cursor = match.end()
    while cursor < len(text) and depth:
        if text[cursor] == "{":
            depth += 1
        elif text[cursor] == "}":
            depth -= 1
        cursor += 1
    if depth:
        raise AssertionError(f"function {name} has unbalanced braces")
    return text[match.end() : cursor - 1]


class NetworkDataContractTests(unittest.TestCase):
    def test_startup_is_independent_of_bap_credentials(self) -> None:
        self.assertIn("wifi_start_saved_connection()", function_body(source("main.c"), "app_main"))
        self.assertNotIn("nvs_flash_erase", function_body(source("wifi.c"), "wifi_init_common"))

    def test_connected_requires_dhcp_address(self) -> None:
        connected = function_body(source("wifi.c"), "wifi_is_connected")
        self.assertIn("ip_info.ip.addr != 0", connected)
        self.assertNotIn("esp_wifi_sta_get_ap_info", connected)

    def test_wifi_power_save_is_disabled_for_tls(self) -> None:
        init = function_body(source("wifi.c"), "wifi_init_common")
        self.assertIn("esp_wifi_set_ps(WIFI_PS_NONE)", init)

    def test_price_waits_for_time_and_owns_https_transport(self) -> None:
        price = source("price.c")
        task = function_body(price, "price_task")
        fetch = function_body(price, "price_fetch_once")
        self.assertIn("wifi_is_time_ready()", task)
        self.assertIn("wifi_https_acquire(15000)", fetch)
        self.assertGreaterEqual(fetch.count("wifi_https_release()"), 2)

    def test_mempool_uses_shared_https_transport(self) -> None:
        fetch = function_body(source("mempool.c"), "mempool_fetch_once")
        self.assertIn("wifi_https_acquire(30000)", fetch)
        self.assertGreaterEqual(fetch.count("wifi_https_release()"), 2)


if __name__ == "__main__":
    unittest.main()
