# SPDX-License-Identifier: GPL-3.0-or-later
# Hardware component: reads an INMP441 I2S microphone and runs the beat detector.
idf_component_register(
    SRCS "src/microphone.cpp"
    INCLUDE_DIRS "include"
    REQUIRES beat esp_driver_i2s esp_driver_gpio esp_timer freertos log
)
