// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "board.hpp"

// The firmware modes. Each one runs forever.
void run_bringup(Board& board);
void run_game(Board& board);
