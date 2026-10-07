# SPDX-License-Identifier: GPL-3.0-or-later
# Pure C++ component. It must not depend on ESP-IDF, so the host tests and the simulator can build it.
idf_component_register(
    SRCS "src/beat_detector.cpp"
    INCLUDE_DIRS "include"
)
