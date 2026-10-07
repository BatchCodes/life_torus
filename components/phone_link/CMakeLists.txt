# SPDX-License-Identifier: GPL-3.0-or-later
# protocol.cpp is pure C++, so the host tests can build it. phone_link.cpp and dns.cpp need the
# ESP-IDF Wi-Fi and HTTP server.
if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS "src/protocol.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game frame
    )
else()
    idf_component_register(
        SRCS "src/protocol.cpp" "src/phone_link.cpp" "src/dns.cpp"
        INCLUDE_DIRS "include"
        REQUIRES game frame esp_wifi esp_netif esp_event esp_http_server lwip log esp_hw_support
    )
endif()
