# SPDX-License-Identifier: GPL-3.0-or-later
# Host tests for the pure C++ components. Build and run them on the ESP-IDF linux target.
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(COMPONENTS main)
project(host_tests)
