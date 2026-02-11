idf_component_register(
    SRCS "ota_update.c" "mempool.c" "block.c" "clock.c" "price.c" "Nevan_RUS_96.c" "untyped_96.c" "montserrat_120.c" "montserrat_140.c" "clock-solid-full.c" "cube-solid-full.c" "cubes-solid-full.c" "bap_client.c" "bap_parser.c" "bap_uart.c" "bap_protocol.c" "settings.c" "home.c" "wifi.c" "night.c"
        "loading.c"
        "waveshare_rgb_lcd_port.c"
        "main.c"
        "lvgl_port.c"
        "assets/lvgl_logo.c"
        "assets/temperature_icon.c"
        "assets/fan.c"
        "assets/star.c"
        "angelwish_96.c"
    INCLUDE_DIRS ".")

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
