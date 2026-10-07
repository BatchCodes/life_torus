// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "frame/image.hpp"
#include "game/config.hpp"
#include "life/grid.hpp"
#include "life/simulation.hpp"
#include "patterns/shape.hpp"

namespace game {

enum class Button : uint8_t {
    kUp,
    kDown,
    kLeft,
    kRight,
    kA,
    kB,
    kX,
    kY,
    kL,
    kR,
    kSelect,
    kStart,
};

enum class Mode : uint8_t {
    kRun,
    kPause,
    kTransition,  // Wipe to a new preset. Then run.
    kKoCode,      // The ko code effect. Then back to run or pause.
};

// The game logic: run and pause, the cursor, the presets, the unattended-play rules and the
// ko code. It never reads a clock. Each call takes the current time in milliseconds.
class Game {
public:
    explicit Game(const GameConfig& config);

    // Loads a random preset with a wipe from an empty board.
    void start(uint32_t now_ms);
    void press(Button button, uint32_t now_ms);
    void release(Button button, uint32_t now_ms);
    // Call often, for example every 10 ms.
    void tick(uint32_t now_ms);
    void render(frame::Image& image, uint32_t now_ms) const;

    Mode mode() const { return mode_; }
    const life::Simulation& simulation() const { return simulation_; }
    int cursor_x() const { return cursor_x_; }
    int cursor_y() const { return cursor_y_; }
    int shape_index() const { return shape_index_; }
    const char* shape_name() const;
    int preset_index() const { return preset_index_; }
    const char* preset_name() const;
    const GameConfig& config() const { return config_; }
    void set_config(const GameConfig& config);

private:
    static constexpr int kRepeatWindow = 32;

    void load_preset(int index);
    void auto_load(uint32_t now_ms);
    void advance(uint32_t now_ms);
    void handle_pause_button(Button button, uint32_t now_ms);
    void move_cursor(Button button, uint32_t now_ms);
    void set_shape(int index);
    void edit(bool stamp);
    void enter_run(uint32_t now_ms);
    // Returns true if this press completes the ko code. Then the press has no other effect.
    bool ko_code_press(Button button, uint32_t now_ms);

    GameConfig config_;
    life::Rng rng_;
    life::Simulation simulation_;
    Mode mode_ = Mode::kPause;

    int preset_index_ = 0;
    int shape_index_ = 0;
    patterns::Shape shape_;
    int cursor_x_ = life::kWidth / 2;
    int cursor_y_ = life::kHeight / 2;
    uint32_t cursor_moved_ms_ = 0;

    uint32_t last_step_ms_ = 0;
    uint32_t last_input_ms_ = 0;
    uint32_t generations_since_input_ = 0;
    uint32_t settled_generations_ = 0;
    life::Grid two_back_;  // The board two generations ago, for the period 2 check.
    int history_ = 0;      // Generations since the last load or edit, up to 2.
    uint32_t repeat_generations_ = 0;
    std::array<uint32_t, kRepeatWindow> recent_hashes_{};
    int recent_count_ = 0;
    int recent_next_ = 0;

    frame::Image transition_from_;
    uint32_t transition_start_ms_ = 0;

    bool dpad_held_ = false;
    Button held_button_ = Button::kUp;
    uint32_t next_repeat_ms_ = 0;

    int next_ko_ = 0;
    uint32_t ko_last_ms_ = 0;
    bool ko_resume_run_ = false;
    life::Grid ko_board_;
    uint32_t ko_start_ms_ = 0;
};

}  // namespace game
