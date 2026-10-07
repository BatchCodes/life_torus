// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "game/config.hpp"
#include "settings/settings.hpp"

// The settings defaults from Kconfig. The saved settings in NVS override them.
settings::Settings settings_from_kconfig();

// The game options that are not run-time settings, from Kconfig, plus the settings.
game::GameConfig game_config(const settings::Settings& s);
