# SPDX-License-Identifier: GPL-3.0-or-later
# Host tests for the pure C++ components. Build and run them on the ESP-IDF linux target.
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/life"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/patterns"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/frame"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/game"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/panel_map"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gamepad_input"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/display_modes"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/settings"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/phone_link"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(COMPONENTS main)
project(host_tests)
