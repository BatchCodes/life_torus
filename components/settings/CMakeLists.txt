# SPDX-License-Identifier: GPL-3.0-or-later
# settings.cpp is pure C++, so the host tests can build it. nvs_store.cpp needs ESP-IDF NVS.
if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS "src/settings.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game panel_map
    )
else()
    idf_component_register(
        SRCS "src/settings.cpp" "src/nvs_store.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game panel_map nvs_flash log
    )
endif()
