# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")

# Optional local settings that git ignores, for example other GPIO pins or the panel layout.
# Copy sdkconfig.local.example to sdkconfig.local. CI and explicit
# -D SDKCONFIG_DEFAULTS builds do not use it.
if(NOT DEFINED SDKCONFIG_DEFAULTS AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.local")
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.local")
    # Defaults only fill in values that sdkconfig does not have yet.
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig" AND
       "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.local" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
        message(WARNING "sdkconfig.local is newer than sdkconfig. Delete sdkconfig to apply it.")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(life_torus)
