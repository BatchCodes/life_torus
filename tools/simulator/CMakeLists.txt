# SPDX-License-Identifier: GPL-3.0-or-later
# Browser simulator: compiles the pure components to WebAssembly with Emscripten.
# Build with scripts/ci.sh simulator (native emsdk) or scripts/simulator_docker.sh (Docker).
cmake_minimum_required(VERSION 3.22)
project(life_torus_simulator CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this project with emcmake (Emscripten).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPONENTS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../components")
add_executable(life_torus
    bindings.cpp
    "${COMPONENTS_DIR}/life/src/grid.cpp"
    "${COMPONENTS_DIR}/life/src/simulation.cpp"
    "${COMPONENTS_DIR}/patterns/src/shape.cpp"
    "${COMPONENTS_DIR}/patterns/src/library.cpp"
    "${COMPONENTS_DIR}/patterns/src/presets.cpp"
    "${COMPONENTS_DIR}/frame/src/image.cpp"
    "${COMPONENTS_DIR}/frame/src/ko_code.cpp"
    "${COMPONENTS_DIR}/game/src/game.cpp"
    "${COMPONENTS_DIR}/game/src/ko_code.cpp"
    "${COMPONENTS_DIR}/display_modes/src/font.cpp"
    "${COMPONENTS_DIR}/display_modes/src/effects.cpp"
    "${COMPONENTS_DIR}/display_modes/src/mode_manager.cpp"
)
target_include_directories(life_torus PRIVATE
    "${COMPONENTS_DIR}/life/include"
    "${COMPONENTS_DIR}/patterns/include"
    "${COMPONENTS_DIR}/frame/include"
    "${COMPONENTS_DIR}/game/include"
    "${COMPONENTS_DIR}/display_modes/include"
)
target_compile_options(life_torus PRIVATE -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti)
target_link_options(life_torus PRIVATE
    -O2
    -sMODULARIZE=1
    -sEXPORT_NAME=createLifeTorus
    -sENVIRONMENT=web
    -sSINGLE_FILE=1
    -sEXPORTED_FUNCTIONS=_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=cwrap,UTF8ToString,HEAPU8
)
# The page loads the module from web/, next to index.html.
set_target_properties(life_torus PROPERTIES
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/web"
)
