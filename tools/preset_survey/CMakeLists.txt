# SPDX-License-Identifier: GPL-3.0-or-later
# Computer tool: runs each preset board and prints how long it stays interesting.
# Build and run:
#   cmake -S tools/preset_survey -B tools/preset_survey/build && cmake --build tools/preset_survey/build
#   tools/preset_survey/build/preset_survey
cmake_minimum_required(VERSION 3.22)
project(preset_survey CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPONENTS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../components")
add_executable(preset_survey
    main.cpp
    "${COMPONENTS_DIR}/life/src/grid.cpp"
    "${COMPONENTS_DIR}/life/src/simulation.cpp"
    "${COMPONENTS_DIR}/patterns/src/shape.cpp"
    "${COMPONENTS_DIR}/patterns/src/library.cpp"
    "${COMPONENTS_DIR}/patterns/src/presets.cpp"
)
target_include_directories(preset_survey PRIVATE
    "${COMPONENTS_DIR}/life/include"
    "${COMPONENTS_DIR}/patterns/include"
)
target_compile_options(preset_survey PRIVATE -Wall -Wextra -Werror)
