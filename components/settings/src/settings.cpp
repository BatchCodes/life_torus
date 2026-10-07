// SPDX-License-Identifier: GPL-3.0-or-later
#include "settings/settings.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace settings {

namespace {

uint32_t limit(uint32_t value, uint32_t low, uint32_t high) {
    return value < low ? low : (value > high ? high : value);
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// Appends text to out at *used. Returns false if it does not fit.
bool append(char* out, size_t size, size_t* used, const char* text) {
    const size_t length = std::strlen(text);
    if (*used + length + 1 > size) {
        return false;
    }
    std::memcpy(out + *used, text, length + 1);
    *used += length;
    return true;
}

bool append_number(char* out, size_t size, size_t* used, const char* key, uint32_t value) {
    char item[48];
    std::snprintf(item, sizeof(item), "%s%s=%lu", *used > 0 ? "&" : "", key,
                  static_cast<unsigned long>(value));
    return append(out, size, used, item);
}

bool append_encoded(char* out, size_t size, size_t* used, const char* key, const char* value) {
    char item[kMaxPassword * 3 + 32];
    size_t n = static_cast<size_t>(std::snprintf(item, sizeof(item), "&%s=", key));
    for (const char* p = value; *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            item[n++] = static_cast<char>(c);
        } else {
            n += static_cast<size_t>(std::snprintf(item + n, 4, "%%%02X", c));
        }
    }
    item[n] = '\0';
    return append(out, size, used, item);
}

bool parse_number(const char* value, uint32_t& out) {
    if (*value == '\0') {
        return false;
    }
    char* end = nullptr;
    const unsigned long n = std::strtoul(value, &end, 10);
    if (*end != '\0') {
        return false;
    }
    out = static_cast<uint32_t>(n);
    return true;
}

bool parse_bool(const char* value, bool& out) {
    uint32_t n = 0;
    if (!parse_number(value, n) || n > 1) {
        return false;
    }
    out = n == 1;
    return true;
}

}  // namespace

void clamp(Settings& s) {
    s.step_ms = limit(s.step_ms, 20, 5000);
    s.settled_limit = limit(s.settled_limit, 1, 100000);
    s.no_input_limit = limit(s.no_input_limit, 1, 1000000);
    s.repeat_limit = limit(s.repeat_limit, 0, 100000);
    s.pause_timeout_ms = limit(s.pause_timeout_ms, 1000, 3600000);
    s.random_percent = limit(s.random_percent, 1, 99);
    s.intensity = limit(s.intensity, 0, 15);
    s.boards = limit(s.boards, 1, 16);
    s.colour = limit(s.colour, 0, 0xFFFFFF);
    s.brightness = limit(s.brightness, 1, 100);
    s.led_current_ma = limit(s.led_current_ma, 500, 20000);
    s.beat_sensitivity = limit(s.beat_sensitivity, 1, 10);
    s.password[kMaxPassword] = '\0';
}

size_t to_text(const Settings& s, char* out, size_t size, bool include_password) {
    if (size == 0) {
        return 0;
    }
    out[0] = '\0';
    size_t used = 0;
    const panel_map::PanelConfig& p = s.panel;
    const bool ok =
        append_number(out, size, &used, "step_ms", s.step_ms) &&
        append_number(out, size, &used, "settled_limit", s.settled_limit) &&
        append_number(out, size, &used, "no_input_limit", s.no_input_limit) &&
        append_number(out, size, &used, "repeat_limit", s.repeat_limit) &&
        append_number(out, size, &used, "pause_timeout_ms", s.pause_timeout_ms) &&
        append_number(out, size, &used, "random_percent", s.random_percent) &&
        append_number(out, size, &used, "cylinder", s.cylinder) &&
        append_number(out, size, &used, "ws2812", s.ws2812) &&
        append_number(out, size, &used, "boards", s.boards) &&
        append_number(out, size, &used, "intensity", s.intensity) &&
        append_number(out, size, &used, "brightness_levels", s.brightness_levels) &&
        append_number(out, size, &used, "reverse_ring", p.reverse_ring) &&
        append_number(out, size, &used, "flip_boards", p.flip_boards) &&
        append_number(out, size, &used, "zigzag", p.zigzag) &&
        append_number(out, size, &used, "block_transpose", p.block_transpose) &&
        append_number(out, size, &used, "block_flip_x", p.block_flip_x) &&
        append_number(out, size, &used, "block_flip_y", p.block_flip_y) &&
        append_number(out, size, &used, "pixel_start", static_cast<uint32_t>(s.pixels.start)) &&
        append_number(out, size, &used, "pixel_rows", s.pixels.rows) &&
        append_number(out, size, &used, "pixel_serpentine", s.pixels.serpentine) &&
        append_number(out, size, &used, "pixel_zigzag", s.pixels.zigzag) &&
        append_number(out, size, &used, "pixel_reverse_ring", s.pixels.reverse_ring) &&
        append_number(out, size, &used, "single_colour", s.single_colour) &&
        append_number(out, size, &used, "colour", s.colour) &&
        append_number(out, size, &used, "brightness", s.brightness) &&
        append_number(out, size, &used, "led_current_ma", s.led_current_ma) &&
        append_number(out, size, &used, "beat_sync", s.beat_sync) &&
        append_number(out, size, &used, "beat_sensitivity", s.beat_sensitivity) &&
        (!include_password || append_encoded(out, size, &used, "password", s.password));
    return ok ? used : 0;
}

bool url_decode(const char* in, size_t in_length, char* out, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < in_length; ++i) {
        char c = in[i];
        if (c == '+') {
            c = ' ';
        } else if (c == '%') {
            if (i + 2 >= in_length) {
                return false;
            }
            const int high = hex_value(in[i + 1]);
            const int low = hex_value(in[i + 2]);
            if (high < 0 || low < 0) {
                return false;
            }
            c = static_cast<char>(high * 16 + low);
            i += 2;
        }
        if (n + 1 >= size) {
            return false;
        }
        out[n++] = c;
    }
    out[n] = '\0';
    return true;
}

bool from_text(const char* text, Settings& s) {
    Settings result = s;
    const char* p = text;
    while (*p != '\0') {
        const char* end = std::strchr(p, '&');
        const size_t item_length = end != nullptr ? static_cast<size_t>(end - p) : std::strlen(p);
        const char* equals = static_cast<const char*>(std::memchr(p, '=', item_length));
        if (equals == nullptr) {
            return false;
        }
        char key[24];
        const size_t key_length = static_cast<size_t>(equals - p);
        if (key_length == 0 || key_length >= sizeof(key)) {
            return false;
        }
        std::memcpy(key, p, key_length);
        key[key_length] = '\0';
        char value[kMaxPassword + 1];
        if (!url_decode(equals + 1, item_length - key_length - 1, value, sizeof(value))) {
            return false;
        }

        bool ok = true;
        panel_map::PanelConfig& panel = result.panel;
        if (std::strcmp(key, "step_ms") == 0) {
            ok = parse_number(value, result.step_ms);
        } else if (std::strcmp(key, "settled_limit") == 0) {
            ok = parse_number(value, result.settled_limit);
        } else if (std::strcmp(key, "no_input_limit") == 0) {
            ok = parse_number(value, result.no_input_limit);
        } else if (std::strcmp(key, "repeat_limit") == 0) {
            ok = parse_number(value, result.repeat_limit);
        } else if (std::strcmp(key, "pause_timeout_ms") == 0) {
            ok = parse_number(value, result.pause_timeout_ms);
        } else if (std::strcmp(key, "random_percent") == 0) {
            ok = parse_number(value, result.random_percent);
        } else if (std::strcmp(key, "cylinder") == 0) {
            ok = parse_bool(value, result.cylinder);
        } else if (std::strcmp(key, "ws2812") == 0) {
            ok = parse_bool(value, result.ws2812);
        } else if (std::strcmp(key, "pixel_start") == 0) {
            uint32_t start = 0;
            ok = parse_number(value, start) && start <= 3;
            result.pixels.start = static_cast<pixel_map::Start>(start);
        } else if (std::strcmp(key, "pixel_rows") == 0) {
            ok = parse_bool(value, result.pixels.rows);
        } else if (std::strcmp(key, "pixel_serpentine") == 0) {
            ok = parse_bool(value, result.pixels.serpentine);
        } else if (std::strcmp(key, "pixel_zigzag") == 0) {
            ok = parse_bool(value, result.pixels.zigzag);
        } else if (std::strcmp(key, "pixel_reverse_ring") == 0) {
            ok = parse_bool(value, result.pixels.reverse_ring);
        } else if (std::strcmp(key, "single_colour") == 0) {
            ok = parse_bool(value, result.single_colour);
        } else if (std::strcmp(key, "colour") == 0) {
            ok = parse_number(value, result.colour);
        } else if (std::strcmp(key, "brightness") == 0) {
            ok = parse_number(value, result.brightness);
        } else if (std::strcmp(key, "led_current_ma") == 0) {
            ok = parse_number(value, result.led_current_ma);
        } else if (std::strcmp(key, "boards") == 0) {
            ok = parse_number(value, result.boards);
        } else if (std::strcmp(key, "intensity") == 0) {
            ok = parse_number(value, result.intensity);
        } else if (std::strcmp(key, "brightness_levels") == 0) {
            ok = parse_bool(value, result.brightness_levels);
        } else if (std::strcmp(key, "reverse_ring") == 0) {
            ok = parse_bool(value, panel.reverse_ring);
        } else if (std::strcmp(key, "flip_boards") == 0) {
            ok = parse_bool(value, panel.flip_boards);
        } else if (std::strcmp(key, "zigzag") == 0) {
            ok = parse_bool(value, panel.zigzag);
        } else if (std::strcmp(key, "block_transpose") == 0) {
            ok = parse_bool(value, panel.block_transpose);
        } else if (std::strcmp(key, "block_flip_x") == 0) {
            ok = parse_bool(value, panel.block_flip_x);
        } else if (std::strcmp(key, "block_flip_y") == 0) {
            ok = parse_bool(value, panel.block_flip_y);
        } else if (std::strcmp(key, "beat_sync") == 0) {
            ok = parse_bool(value, result.beat_sync);
        } else if (std::strcmp(key, "beat_sensitivity") == 0) {
            ok = parse_number(value, result.beat_sensitivity);
        } else if (std::strcmp(key, "password") == 0) {
            ok = value[0] != '\0';
            if (ok) {
                std::strncpy(result.password, value, kMaxPassword);
                result.password[kMaxPassword] = '\0';
            }
        }
        if (!ok) {
            return false;
        }
        if (end == nullptr) {
            break;
        }
        p = end + 1;
    }
    clamp(result);
    s = result;
    return true;
}

void apply(const Settings& s, game::GameConfig& config) {
    config.step_ms = s.step_ms;
    config.settled_limit = s.settled_limit;
    config.no_input_limit = s.no_input_limit;
    config.repeat_limit = s.repeat_limit;
    config.pause_timeout_ms = s.pause_timeout_ms;
    config.random_percent = static_cast<int>(s.random_percent);
    config.default_edge_mode = s.cylinder ? life::EdgeMode::kCylinder : life::EdgeMode::kTorus;
    config.brightness_levels = s.brightness_levels;
}

}  // namespace settings
