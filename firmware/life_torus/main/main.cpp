// SPDX-License-Identifier: GPL-3.0-or-later
#include "app_settings.hpp"
#include "board.hpp"
#include "esp_log.h"
#include "modes.hpp"
#include "sdkconfig.h"
#include "settings/nvs_store.hpp"

namespace {

constexpr const char* kTag = "life_torus";

Board g_board;
settings::Settings g_settings;

}  // namespace

extern "C" void app_main() {
    g_settings = settings_from_kconfig();
    if (settings::nvs_init() == ESP_OK) {
        settings::load(g_settings);
    } else {
        ESP_LOGW(kTag, "NVS init failed. Saved settings are not available.");
    }
    if (g_board.init(g_settings) != ESP_OK) {
        ESP_LOGE(kTag, "display init failed. Check the GPIO options in menuconfig.");
        return;
    }
#if CONFIG_LIFE_MODE_BRINGUP
    run_bringup(g_board, g_settings);
#else
    run_game(g_board, g_settings);
#endif
}
