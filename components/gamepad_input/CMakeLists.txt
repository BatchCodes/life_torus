# SPDX-License-Identifier: GPL-3.0-or-later
# report.cpp is pure C++, so the host tests can build it. usb_gamepad.cpp needs the ESP32-S3 USB host.
if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS "src/report.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game
    )
else()
    idf_component_register(
        SRCS "src/report.cpp" "src/usb_gamepad.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game freertos log
    )
endif()
