# SPDX-License-Identifier: GPL-3.0-or-later
# debouncer.cpp is pure C++, so the host tests can build it. mode_button.cpp reads the GPIO.
if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS "src/debouncer.cpp"
        INCLUDE_DIRS "include"
    )
else()
    idf_component_register(
        SRCS "src/debouncer.cpp" "src/mode_button.cpp"
        INCLUDE_DIRS "include"
        REQUIRES esp_driver_gpio
    )
endif()
