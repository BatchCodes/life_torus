# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp" "board.cpp" "app_settings.cpp" "bringup.cpp" "game_app.cpp"
    PRIV_REQUIRES life patterns frame game panel_map max7219_chain gamepad_input settings
                  display_modes phone_link microphone esp_timer
                  esp_hw_support esp_system log freertos
    # The phone app, served by phone_link.
    EMBED_FILES "../../../tools/phone_app/index.html" "../../../tools/phone_app/app.js"
                "../../../tools/phone_app/style.css" "../../../tools/phone_app/listen.js"
)
