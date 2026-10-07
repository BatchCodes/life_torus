// SPDX-License-Identifier: GPL-3.0-or-later
#include "phone_link/protocol.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace phone_link {

namespace {

constexpr const char* kButtonNames[] = {"up", "down", "left", "right", "a",      "b",
                                        "x",  "y",    "l",    "r",     "select", "start"};

bool starts_with(const char* data, size_t length, const char* word, const char** rest,
                 size_t* rest_length) {
    const size_t n = std::strlen(word);
    if (length < n || std::memcmp(data, word, n) != 0) {
        return false;
    }
    if (length == n) {
        *rest = data + n;
        *rest_length = 0;
        return true;
    }
    if (data[n] != ' ') {
        return false;
    }
    *rest = data + n + 1;
    *rest_length = length - n - 1;
    return true;
}

// Reads one decimal number (with an optional minus sign) and moves past one space after it.
bool read_int(const char*& p, const char* end, int& out) {
    bool negative = false;
    if (p < end && *p == '-') {
        negative = true;
        ++p;
    }
    if (p >= end || *p < '0' || *p > '9') {
        return false;
    }
    long value = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        value = value * 10 + (*p - '0');
        if (value > 1000000) {
            return false;
        }
        ++p;
    }
    if (p < end) {
        if (*p != ' ') {
            return false;
        }
        ++p;
    }
    out = static_cast<int>(negative ? -value : value);
    return true;
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

bool decode(const char* in, size_t length, char* out, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < length; ++i) {
        char c = in[i];
        if (c == '%') {
            if (i + 2 >= length) {
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

}  // namespace

bool url_decode(const char* in, size_t length, char* out, size_t size) {
    return decode(in, length, out, size);
}

const char* button_name(game::Button button) {
    return kButtonNames[static_cast<int>(button)];
}

bool button_from_name(const char* name, size_t length, game::Button& out) {
    for (int i = 0; i < static_cast<int>(std::size(kButtonNames)); ++i) {
        if (std::strlen(kButtonNames[i]) == length &&
            std::memcmp(kButtonNames[i], name, length) == 0) {
            out = static_cast<game::Button>(i);
            return true;
        }
    }
    return false;
}

bool parse_command(const char* data, size_t length, Command& out) {
    out = Command{};
    const char* rest = nullptr;
    size_t rest_length = 0;
    const char* end = nullptr;

    if (starts_with(data, length, "press", &rest, &rest_length)) {
        out.type = CommandType::kPress;
        return button_from_name(rest, rest_length, out.button);
    }
    if (starts_with(data, length, "release", &rest, &rest_length)) {
        out.type = CommandType::kRelease;
        return button_from_name(rest, rest_length, out.button);
    }
    if (starts_with(data, length, "cell", &rest, &rest_length)) {
        out.type = CommandType::kCell;
        end = rest + rest_length;
        return read_int(rest, end, out.a) && read_int(rest, end, out.b) && rest == end;
    }
    if (starts_with(data, length, "preset", &rest, &rest_length)) {
        out.type = CommandType::kPreset;
        end = rest + rest_length;
        return read_int(rest, end, out.a) && rest == end;
    }
    if (starts_with(data, length, "mode", &rest, &rest_length)) {
        out.type = CommandType::kMode;
        end = rest + rest_length;
        return read_int(rest, end, out.a) && rest == end;
    }
    if (starts_with(data, length, "speed", &rest, &rest_length)) {
        out.type = CommandType::kSpeed;
        end = rest + rest_length;
        return read_int(rest, end, out.a) && rest == end;
    }
    if (starts_with(data, length, "text", &rest, &rest_length)) {
        out.type = CommandType::kText;
        return decode(rest, rest_length, out.text, sizeof(out.text));
    }
    if (starts_with(data, length, "settings", &rest, &rest_length)) {
        out.type = CommandType::kSettings;
        if (rest_length == 0 || rest_length > kMaxCommandText) {
            return false;
        }
        std::memcpy(out.text, rest, rest_length);
        out.text[rest_length] = '\0';
        return true;
    }
    if (starts_with(data, length, "get_settings", &rest, &rest_length)) {
        out.type = CommandType::kGetSettings;
        return rest_length == 0;
    }
    return false;
}

void pack_frame(const frame::Image& image, uint8_t* out) {
    out[0] = 'F';
    for (int i = 0; i < life::kWidth * life::kHeight / 4; ++i) {
        uint8_t byte = 0;
        for (int k = 0; k < 4; ++k) {
            const int cell = i * 4 + k;
            byte |= static_cast<uint8_t>(
                static_cast<uint8_t>(image.get(cell % life::kWidth, cell / life::kWidth))
                << (k * 2));
        }
        out[1 + i] = byte;
    }
}

size_t url_encode(const char* in, char* out, size_t size) {
    size_t n = 0;
    for (const char* p = in; *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        const bool plain = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                           (c >= 'A' && c <= 'Z') || c == '-' || c == '_' || c == '.' || c == '~';
        const size_t needed = plain ? 1 : 3;
        if (n + needed + 1 > size) {
            return 0;
        }
        if (plain) {
            out[n++] = static_cast<char>(c);
        } else {
            n += static_cast<size_t>(std::snprintf(out + n, 4, "%%%02X", c));
        }
    }
    if (n + 1 > size) {
        return 0;
    }
    out[n] = '\0';
    return n;
}

const char* Sessions::create(const uint8_t random[16]) {
    auto& token = tokens_[next_];
    for (int i = 0; i < 16; ++i) {
        std::snprintf(token.data() + i * 2, 3, "%02x", random[i]);
    }
    token[kTokenLength] = '\0';
    next_ = (next_ + 1) % kMax;
    return token.data();
}

bool Sessions::valid(const char* token, size_t length) const {
    if (length != kTokenLength) {
        return false;
    }
    for (const auto& t : tokens_) {
        if (t[0] != '\0' && std::memcmp(t.data(), token, kTokenLength) == 0) {
            return true;
        }
    }
    return false;
}

void Sessions::clear() {
    for (auto& t : tokens_) {
        t[0] = '\0';
    }
}

bool password_matches(const char* expected, const char* given, size_t given_length) {
    const size_t expected_length = std::strlen(expected);
    uint8_t difference = expected_length == given_length ? 0 : 1;
    for (size_t i = 0; i < given_length; ++i) {
        const char e = i < expected_length ? expected[i] : 0;
        difference |= static_cast<uint8_t>(e ^ given[i]);
    }
    return difference == 0;
}

bool find_cookie(const char* header, const char* name, char* out, size_t size) {
    const size_t name_length = std::strlen(name);
    const char* p = header;
    while (*p != '\0') {
        while (*p == ' ' || *p == ';') {
            ++p;
        }
        const char* end = std::strchr(p, ';');
        const size_t item_length = end != nullptr ? static_cast<size_t>(end - p) : std::strlen(p);
        if (item_length > name_length && std::memcmp(p, name, name_length) == 0 &&
            p[name_length] == '=') {
            const size_t value_length = item_length - name_length - 1;
            if (value_length + 1 > size) {
                return false;
            }
            std::memcpy(out, p + name_length + 1, value_length);
            out[value_length] = '\0';
            return true;
        }
        if (end == nullptr) {
            break;
        }
        p = end + 1;
    }
    return false;
}

}  // namespace phone_link
