// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "frame/image.hpp"
#include "game/buttons.hpp"

namespace phone_link {

// Messages from the phone app, one per WebSocket text message:
//   press NAME / release NAME   controller button (up, down, left, right, a, b, x, y, l, r,
//                               select, start)
//   cell X Y                    toggle the cell at column X, row Y
//   preset N                    load preset N
//   mode N                      display mode N (0: Game of Life)
//   text TEXT                   scrolling text message (percent-encoded)
//   speed N                     display mode speed, 1 to 10
//   settings KEY=VALUE&...      change settings and save them
//   get_settings                ask for the settings
enum class CommandType : uint8_t {
    kPress,
    kRelease,
    kCell,
    kPreset,
    kMode,
    kText,
    kSpeed,
    kSettings,
    kGetSettings,
};

constexpr size_t kMaxCommandText = 800;

struct Command {
    CommandType type;
    game::Button button;
    int a;
    int b;
    char text[kMaxCommandText + 1];
};

bool parse_command(const char* data, size_t length, Command& out);

const char* button_name(game::Button button);
bool button_from_name(const char* name, size_t length, game::Button& out);

// A display frame for the phone: 'F', the width and the height (one byte each), then 2 bits per
// cell (the brightness level), 4 cells per byte, row by row, the first cell in the low bits.
constexpr size_t kMaxFrameSize = 3 + life::kMaxWidth * life::kHeight / 4;
size_t frame_size();
// Writes frame_size() bytes.
void pack_frame(const frame::Image& image, uint8_t* out);

// Percent decoding ("a%20b" to "a b"). Returns false if the text is not valid or does not fit.
bool url_decode(const char* in, size_t length, char* out, size_t size);

// Percent encoding for status and settings values. Returns the length, or 0 if it does not fit.
size_t url_encode(const char* in, char* out, size_t size);

// Login sessions: a random token for each phone that gave the correct password.
class Sessions {
public:
    static constexpr int kMax = 8;
    static constexpr size_t kTokenLength = 32;

    // Makes a new token from 16 random bytes. The oldest session ends when all slots are used.
    const char* create(const uint8_t random[16]);
    bool valid(const char* token, size_t length) const;
    void clear();

private:
    std::array<std::array<char, kTokenLength + 1>, kMax> tokens_{};
    int next_ = 0;
};

// Compares two passwords in a time that does not depend on where they differ.
bool password_matches(const char* expected, const char* given, size_t given_length);

// Finds the value of cookie `name` in a Cookie header. Returns false if it is not there.
bool find_cookie(const char* header, const char* name, char* out, size_t size);

}  // namespace phone_link
