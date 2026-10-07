# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp" "board.cpp" "bringup.cpp"
    PRIV_REQUIRES life patterns frame game panel_map max7219_chain gamepad_input esp_timer
                  esp_hw_support log freertos
)
