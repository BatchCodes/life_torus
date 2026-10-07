# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "test_main.cpp" "test_life.cpp"
    PRIV_REQUIRES unity life
    WHOLE_ARCHIVE
)
