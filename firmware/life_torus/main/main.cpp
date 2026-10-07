// SPDX-License-Identifier: GPL-3.0-or-later
#include "board.hpp"
#include "esp_log.h"
#include "modes.hpp"
#include "sdkconfig.h"

namespace {

constexpr const char* kTag = "life_torus";

Board g_board;

}  // namespace

extern "C" void app_main() {
    if (g_board.init() != ESP_OK) {
        ESP_LOGE(kTag, "display init failed. Check the GPIO options in menuconfig.");
        return;
    }
#if CONFIG_LIFE_MODE_BRINGUP
    run_bringup(g_board);
#else
    run_game(g_board);
#endif
}
