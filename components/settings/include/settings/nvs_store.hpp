// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "esp_err.h"
#include "settings/settings.hpp"

namespace settings {

// Initialises NVS. Erases it if its format is old or the partition is full.
esp_err_t nvs_init();
// Applies the saved settings to `s`. Leaves `s` unchanged if nothing is saved.
esp_err_t load(Settings& s);
esp_err_t save(const Settings& s);

}  // namespace settings
