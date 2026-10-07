// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/game.hpp"

#include "patterns/library.hpp"
#include "patterns/presets.hpp"

namespace game {

namespace {

bool is_dpad(Button button) {
    return button == Button::kUp || button == Button::kDown || button == Button::kLeft ||
           button == Button::kRight;
}

}  // namespace

Game::Game(const GameConfig& config) : config_(config), rng_(config.seed) {
    set_shape(0);
}

void Game::set_config(const GameConfig& config) {
    config_ = config;
    rng_ = life::Rng(config.seed);
}

const char* Game::shape_name() const {
    return patterns::cursor_shapes()[shape_index_].name;
}

const char* Game::preset_name() const {
    return patterns::presets()[preset_index_].name;
}

void Game::start(uint32_t now_ms) {
    last_input_ms_ = now_ms;
    transition_from_.fill(frame::Level::kOff);
    auto_load(now_ms);
}

void Game::load_preset(int index) {
    preset_index_ = index;
    life::Grid grid;
    const life::EdgeMode edge_mode = patterns::build_preset(
        patterns::presets()[index], grid, rng_, config_.default_edge_mode, config_.random_percent);
    simulation_.load(grid, edge_mode);
    generations_since_input_ = 0;
    settled_generations_ = 0;
    history_ = 0;
    repeat_generations_ = 0;
    recent_count_ = 0;
    recent_next_ = 0;
}

void Game::auto_load(uint32_t now_ms) {
    // A random preset, not the same as the last one, and never the empty board.
    const auto all = patterns::presets();
    int candidates = 0;
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        if (i != preset_index_ && all[i].kind != patterns::PresetKind::kEmpty) {
            ++candidates;
        }
    }
    int pick = static_cast<int>(rng_.below(static_cast<uint32_t>(candidates)));
    int index = 0;
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        if (i != preset_index_ && all[i].kind != patterns::PresetKind::kEmpty) {
            if (pick == 0) {
                index = i;
                break;
            }
            --pick;
        }
    }

    if (mode_ != Mode::kTransition) {
        frame::draw_life(transition_from_, simulation_, config_.brightness_levels);
    }
    load_preset(index);
    mode_ = Mode::kTransition;
    transition_start_ms_ = now_ms;
}

void Game::enter_run(uint32_t now_ms) {
    mode_ = Mode::kRun;
    last_step_ms_ = now_ms;
    dpad_held_ = false;
}

void Game::advance(uint32_t now_ms) {
    const life::Grid before = simulation_.current();
    simulation_.advance();
    ++generations_since_input_;

    // Settled: nothing moves (this includes an empty board), or the board flips between two
    // states. Compare with the boards one and two generations ago.
    const life::Grid& grid = simulation_.current();
    const bool still = grid == before;
    const bool period_two = history_ >= 1 && grid == two_back_;
    settled_generations_ = (still || period_two) ? settled_generations_ + 1 : 0;
    two_back_ = before;
    if (history_ < 2) {
        ++history_;
    }

    const uint32_t hash = grid.hash();
    bool repeated = false;
    for (int i = 0; i < recent_count_; ++i) {
        if (recent_hashes_[i] == hash) {
            repeated = true;
            break;
        }
    }
    repeat_generations_ = repeated ? repeat_generations_ + 1 : 0;
    recent_hashes_[recent_next_] = hash;
    recent_next_ = (recent_next_ + 1) % kRepeatWindow;
    if (recent_count_ < kRepeatWindow) {
        ++recent_count_;
    }

    if (settled_generations_ >= config_.settled_limit ||
        generations_since_input_ >= config_.no_input_limit ||
        (config_.repeat_limit > 0 && !grid.empty() &&
         repeat_generations_ >= config_.repeat_limit)) {
        auto_load(now_ms);
    }
}

void Game::tick(uint32_t now_ms) {
    switch (mode_) {
        case Mode::kTransition:
            if (now_ms - transition_start_ms_ >= config_.transition_ms) {
                enter_run(now_ms);
            }
            break;
        case Mode::kKoCode:
            if (now_ms - ko_start_ms_ >= config_.ko_effect_ms) {
                if (ko_resume_run_) {
                    enter_run(now_ms);
                } else {
                    mode_ = Mode::kPause;
                }
            }
            break;
        case Mode::kRun:
            if (now_ms - last_step_ms_ >= config_.step_ms) {
                last_step_ms_ = now_ms;
                advance(now_ms);
            }
            break;
        case Mode::kPause:
            if (now_ms - last_input_ms_ >= config_.pause_timeout_ms) {
                enter_run(now_ms);
                break;
            }
            if (dpad_held_ && static_cast<int32_t>(now_ms - next_repeat_ms_) >= 0) {
                move_cursor(held_button_, now_ms);
                next_repeat_ms_ += config_.repeat_interval_ms;
            }
            break;
    }
}

void Game::press(Button button, uint32_t now_ms) {
    if (mode_ == Mode::kKoCode) {
        return;
    }
    if (mode_ == Mode::kTransition) {
        mode_ = Mode::kRun;
    }
    last_input_ms_ = now_ms;
    generations_since_input_ = 0;

    if (ko_code_press(button, now_ms)) {
        return;
    }

    if (mode_ == Mode::kRun) {
        mode_ = Mode::kPause;
        cursor_moved_ms_ = now_ms;
        return;
    }
    handle_pause_button(button, now_ms);
}

void Game::release(Button button, uint32_t /*now_ms*/) {
    if (dpad_held_ && button == held_button_) {
        dpad_held_ = false;
    }
}

void Game::handle_pause_button(Button button, uint32_t now_ms) {
    const int shape_count = static_cast<int>(patterns::cursor_shapes().size());
    const int preset_count = static_cast<int>(patterns::presets().size());
    switch (button) {
        case Button::kUp:
        case Button::kDown:
        case Button::kLeft:
        case Button::kRight:
            move_cursor(button, now_ms);
            dpad_held_ = true;
            held_button_ = button;
            next_repeat_ms_ = now_ms + config_.repeat_delay_ms;
            break;
        case Button::kA:
            edit(true);
            break;
        case Button::kB:
            edit(false);
            break;
        case Button::kL:
            set_shape((shape_index_ + shape_count - 1) % shape_count);
            break;
        case Button::kR:
            set_shape((shape_index_ + 1) % shape_count);
            break;
        case Button::kX:
            shape_ = patterns::rotate(shape_);
            break;
        case Button::kY:
            shape_ = patterns::mirror(shape_);
            break;
        case Button::kSelect:
            load_preset((preset_index_ + 1) % preset_count);
            break;
        case Button::kStart:
            enter_run(now_ms);
            break;
    }
    if (!is_dpad(button)) {
        cursor_moved_ms_ = now_ms;
    }
}

void Game::move_cursor(Button button, uint32_t now_ms) {
    switch (button) {
        case Button::kUp:
            cursor_y_ = life::wrap_y(cursor_y_ - 1);
            break;
        case Button::kDown:
            cursor_y_ = life::wrap_y(cursor_y_ + 1);
            break;
        case Button::kLeft:
            cursor_x_ = life::wrap_x(cursor_x_ - 1);
            break;
        case Button::kRight:
            cursor_x_ = life::wrap_x(cursor_x_ + 1);
            break;
        default:
            break;
    }
    cursor_moved_ms_ = now_ms;
}

void Game::set_shape(int index) {
    shape_index_ = index;
    shape_ = patterns::load_shape(index);
}

void Game::edit(bool stamp) {
    life::Grid grid = simulation_.current();
    const life::EdgeMode edge_mode = simulation_.edge_mode();
    if (stamp && patterns::cursor_shapes()[shape_index_].group == patterns::Group::kSingle) {
        grid.toggle(cursor_x_, cursor_y_);
    } else if (stamp) {
        patterns::stamp(grid, shape_, cursor_x_, cursor_y_, edge_mode);
    } else {
        patterns::erase(grid, shape_, cursor_x_, cursor_y_, edge_mode);
    }
    simulation_.edit(grid);
    settled_generations_ = 0;
    history_ = 0;
}

void Game::render(frame::Image& image, uint32_t now_ms) const {
    switch (mode_) {
        case Mode::kKoCode:
            frame::draw_ko_code(image,
                                static_cast<int>((now_ms - ko_start_ms_) / config_.ko_column_ms));
            return;
        case Mode::kTransition: {
            frame::Image next;
            frame::draw_life(next, simulation_, config_.brightness_levels);
            const uint32_t elapsed = now_ms - transition_start_ms_;
            const int columns =
                config_.transition_ms == 0
                    ? life::kWidth
                    : static_cast<int>(elapsed * life::kWidth / config_.transition_ms);
            frame::draw_wipe(image, transition_from_, next, columns);
            return;
        }
        case Mode::kRun:
            frame::draw_life(image, simulation_, config_.brightness_levels);
            return;
        case Mode::kPause: {
            frame::draw_life(image, simulation_, config_.brightness_levels);
            const uint32_t blink = config_.cursor_blink_ms == 0 ? 1 : config_.cursor_blink_ms;
            const bool lit = ((now_ms - cursor_moved_ms_) / blink) % 2 == 0;
            frame::draw_cursor(image, shape_, cursor_x_, cursor_y_, simulation_.edge_mode(), lit);
            return;
        }
    }
}

}  // namespace game
