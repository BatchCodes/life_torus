# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "test_main.cpp" "test_life.cpp" "test_patterns.cpp" "test_frame.cpp" "test_game.cpp" "test_panel_map.cpp" "test_gamepad_report.cpp" "test_display_modes.cpp"
    PRIV_REQUIRES unity life patterns frame game panel_map gamepad_input display_modes
    WHOLE_ARCHIVE
)
