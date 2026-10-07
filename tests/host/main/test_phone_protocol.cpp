// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstring>

#include "phone_link/protocol.hpp"
#include "tests.hpp"
#include "unity.h"

using phone_link::Command;
using phone_link::CommandType;

namespace {

bool parse(const char* text, Command& c) {
    return phone_link::parse_command(text, std::strlen(text), c);
}

void test_button_commands() {
    Command c;
    TEST_ASSERT_TRUE(parse("press start", c));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::kPress), static_cast<int>(c.type));
    TEST_ASSERT_EQUAL(static_cast<int>(game::Button::kStart), static_cast<int>(c.button));
    TEST_ASSERT_TRUE(parse("release up", c));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::kRelease), static_cast<int>(c.type));
    TEST_ASSERT_FALSE(parse("press jump", c));
    TEST_ASSERT_FALSE(parse("press", c));
    TEST_ASSERT_FALSE(parse("pressstart", c));
    for (int i = 0; i < 12; ++i) {
        const auto b = static_cast<game::Button>(i);
        game::Button back;
        const char* name = phone_link::button_name(b);
        TEST_ASSERT_TRUE(phone_link::button_from_name(name, std::strlen(name), back));
        TEST_ASSERT_EQUAL(i, static_cast<int>(back));
    }
}

void test_number_commands() {
    Command c;
    TEST_ASSERT_TRUE(parse("cell 12 31", c));
    TEST_ASSERT_EQUAL(12, c.a);
    TEST_ASSERT_EQUAL(31, c.b);
    TEST_ASSERT_FALSE(parse("cell 12", c));
    TEST_ASSERT_FALSE(parse("cell 12 x", c));
    TEST_ASSERT_FALSE(parse("cell 1 2 3", c));
    TEST_ASSERT_TRUE(parse("mode 3", c));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::kMode), static_cast<int>(c.type));
    TEST_ASSERT_EQUAL(3, c.a);
    TEST_ASSERT_TRUE(parse("speed 10", c));
    TEST_ASSERT_TRUE(parse("preset 2", c));
    TEST_ASSERT_FALSE(parse("mode", c));
}

void test_text_commands() {
    Command c;
    TEST_ASSERT_TRUE(parse("text Hello%20world%21", c));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::kText), static_cast<int>(c.type));
    TEST_ASSERT_EQUAL_STRING("Hello world!", c.text);
    TEST_ASSERT_FALSE(parse("text bad%2", c));
    TEST_ASSERT_TRUE(parse("settings intensity=5&zigzag=1", c));
    TEST_ASSERT_EQUAL_STRING("intensity=5&zigzag=1", c.text);
    TEST_ASSERT_TRUE(parse("get_settings", c));
    TEST_ASSERT_FALSE(parse("unknown 1", c));
}

void test_pack_frame() {
    frame::Image image;
    image.set(0, 0, frame::Level::kBright);
    image.set(1, 0, frame::Level::kDim);
    image.set(63, 31, frame::Level::kNormal);
    uint8_t out[phone_link::kFrameSize];
    phone_link::pack_frame(image, out);
    TEST_ASSERT_EQUAL_HEX8('F', out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x07, out[1]);                           // 3 | 1 << 2
    TEST_ASSERT_EQUAL_HEX8(0x80, out[phone_link::kFrameSize - 1]);  // 2 << 6
    TEST_ASSERT_EQUAL(513, static_cast<int>(phone_link::kFrameSize));
}

void test_url_encode() {
    char out[32];
    TEST_ASSERT_EQUAL(9, static_cast<int>(phone_link::url_encode("a b&c", out, sizeof(out))));
    TEST_ASSERT_EQUAL_STRING("a%20b%26c", out);
    char small[4];
    TEST_ASSERT_EQUAL(0, static_cast<int>(phone_link::url_encode("a b", small, sizeof(small))));
}

void test_sessions() {
    phone_link::Sessions sessions;
    uint8_t random[16];
    for (int i = 0; i < 16; ++i) {
        random[i] = static_cast<uint8_t>(i * 17);
    }
    const char* token = sessions.create(random);
    char copy[40];
    std::strcpy(copy, token);
    TEST_ASSERT_EQUAL(32, static_cast<int>(std::strlen(copy)));
    TEST_ASSERT_TRUE(sessions.valid(copy, 32));
    copy[0] = copy[0] == 'a' ? 'b' : 'a';
    TEST_ASSERT_FALSE(sessions.valid(copy, 32));
    TEST_ASSERT_FALSE(sessions.valid("", 0));
    // The oldest session ends after kMax new ones.
    std::strcpy(copy, token);
    for (int i = 0; i < phone_link::Sessions::kMax; ++i) {
        random[0] = static_cast<uint8_t>(200 + i);
        sessions.create(random);
    }
    TEST_ASSERT_FALSE(sessions.valid(copy, 32));
}

void test_password_and_cookie() {
    TEST_ASSERT_TRUE(phone_link::password_matches("life", "life", 4));
    TEST_ASSERT_FALSE(phone_link::password_matches("life", "lift", 4));
    TEST_ASSERT_FALSE(phone_link::password_matches("life", "lif", 3));
    TEST_ASSERT_FALSE(phone_link::password_matches("life", "lifes", 5));
    char value[40];
    TEST_ASSERT_TRUE(
        phone_link::find_cookie("a=1; session=abc123; b=2", "session", value, sizeof(value)));
    TEST_ASSERT_EQUAL_STRING("abc123", value);
    TEST_ASSERT_FALSE(phone_link::find_cookie("sessions=x", "session", value, sizeof(value)));
}

}  // namespace

void run_phone_protocol_tests() {
    RUN_TEST(test_button_commands);
    RUN_TEST(test_number_commands);
    RUN_TEST(test_text_commands);
    RUN_TEST(test_pack_frame);
    RUN_TEST(test_url_encode);
    RUN_TEST(test_sessions);
    RUN_TEST(test_password_and_cookie);
}
