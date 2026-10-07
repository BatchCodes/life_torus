# SPDX-License-Identifier: GPL-3.0-or-later
# Hardware component: drives the MAX7219 chain over SPI.
idf_component_register(
    SRCS "src/max7219_chain.cpp" "src/refresh.cpp"
    INCLUDE_DIRS "include"
    REQUIRES panel_map esp_driver_spi esp_driver_gpio freertos log
)
