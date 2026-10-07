# SPDX-License-Identifier: GPL-3.0-or-later
# Hardware component: drives WS2812B panels on up to 4 data lines with the RMT peripheral.
idf_component_register(
    SRCS "src/ws2812_chain.cpp" "src/led_encoder.cpp"
    INCLUDE_DIRS "include"
    REQUIRES pixel_map colour esp_driver_rmt log
)
