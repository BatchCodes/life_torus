// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "board.hpp"
#include "settings/settings.hpp"

// The firmware modes. Each one runs forever.
void run_bringup(Board& board, settings::Settings& s);
void run_game(Board& board, settings::Settings& s);
