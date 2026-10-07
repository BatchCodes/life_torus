// SPDX-License-Identifier: GPL-3.0-or-later
//
// Bring-up firmware: test patterns for a new display. docs/hardware/bring-up.md describes each
// pattern and what to look for.
#include <cstdint>

#include "board.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "gamepad_input/usb_gamepad.hpp"
#include "mode_button/mode_button.hpp"
#include "modes.hpp"
#include "sdkconfig.h"
#include "settings/nvs_store.hpp"

namespace {

constexpr const char* kTag = "bringup";

enum class Pattern : int {
    kChipWalk,
    kChipNumbers,
    kColumnSweep,
    kRowSweep,
    kAllOn,
    kLevels,
    kCount,
};

const char* describe(Pattern pattern) {
    switch (pattern) {
        case Pattern::kChipWalk:
            return "chip walk: one 8x8 block at a time, in chain order. Block 0 is the bottom "
                   "block of the first board.";
        case Pattern::kChipNumbers:
            return "chip numbers: each block shows its chain number (0 to 31), upright and "
                   "readable, increasing left to right around the ring.";
        case Pattern::kColumnSweep:
            return "column sweep: one full column moves to the right around the ring.";
        case Pattern::kRowSweep:
            return "row sweep: one full row moves down, on all boards at the same height.";
        case Pattern::kAllOn:
            return "all on: every LED on. Measure the 5 V current. L and R change the intensity.";
        case Pattern::kLevels:
            return "levels: top band bright, middle band normal, bottom band dim. Look for "
                   "flicker or shimmer.";
        case Pattern::kCount:
            break;
    }
    return "";
}

// 3 × 5 digits, one row per byte, bit 2 is the left column.
constexpr uint8_t kDigits[10][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

void draw_digit(frame::Image& image, int digit, int left, int top) {
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if ((kDigits[digit][row] >> (2 - col)) & 1u) {
                image.set(left + col, top + row, frame::Level::kBright);
            }
        }
    }
}

void log_panel(const panel_map::PanelConfig& p) {
    ESP_LOGI(kTag, "panel layout for sdkconfig.local:");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_REVERSE_RING=%s", p.reverse_ring ? "y" : "n");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_FLIP_BOARDS=%s", p.flip_boards ? "y" : "n");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_ZIGZAG=%s", p.zigzag ? "y" : "n");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_BLOCK_TRANSPOSE=%s", p.block_transpose ? "y" : "n");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_BLOCK_FLIP_X=%s", p.block_flip_x ? "y" : "n");
    ESP_LOGI(kTag, "  CONFIG_LIFE_PANEL_BLOCK_FLIP_Y=%s", p.block_flip_y ? "y" : "n");
}

void log_pixels(const pixel_map::PixelConfig& p) {
    constexpr const char* kStarts[] = {"bottom left", "bottom right", "top left", "top right"};
    ESP_LOGI(kTag, "WS2812B layout: start %s, %s, %s, zigzag %s, ring %s",
             kStarts[static_cast<int>(p.start)], p.rows ? "rows" : "columns",
             p.serpentine ? "serpentine" : "same direction", p.zigzag ? "on" : "off",
             p.reverse_ring ? "right to left" : "left to right");
}

// WS2812B: one lit LED walks along the LED order of each data line, with a short tail. It shows
// the start corner and the direction of the panels.
void render_led_walk(Board& board, uint32_t now_ms) {
    std::array<std::vector<colour::Rgb>, pixel_map::kMaxLines> leds;
    const int head = static_cast<int>((now_ms / 40) % pixel_map::kLedsPerPanel);
    for (int line = 0; line < pixel_map::kMaxLines; ++line) {
        leds[line].assign(static_cast<size_t>(pixel_map::line_length(board.pixels(), line)), {});
        for (int k = 0; k < 6 && head - k >= 0; ++k) {
            const uint8_t v = static_cast<uint8_t>(60 - k * 10);
            // Line 1 red, line 2 green, line 3 blue, line 4 white.
            const colour::Rgb c = line == 0   ? colour::Rgb{v, 0, 0}
                                  : line == 1 ? colour::Rgb{0, v, 0}
                                  : line == 2 ? colour::Rgb{0, 0, v}
                                              : colour::Rgb{v, v, v};
            for (size_t panel = 0; panel * pixel_map::kLedsPerPanel < leds[line].size(); ++panel) {
                leds[line][panel * pixel_map::kLedsPerPanel + static_cast<size_t>(head - k)] = c;
            }
        }
    }
    board.show_raw_leds(leds);
}

void render(Board& board, Pattern pattern, uint32_t now_ms) {
    frame::Image image;
    colour::Source source = colour::Source::kGameOfLife;
    switch (pattern) {
        case Pattern::kChipWalk: {
            if (board.ws2812()) {
                render_led_walk(board, now_ms);
                return;
            }
            panel_map::Registers registers{};
            const int chip = static_cast<int>((now_ms / 500) % panel_map::chips());
            for (auto& digit : registers) {
                digit[chip] = 0xFF;
            }
            board.show_raw(registers);
            return;
        }
        case Pattern::kChipNumbers:
            if (board.ws2812()) {
                // The panel number at the top of each panel, upright and readable.
                for (int panel = 0; panel < pixel_map::panels(); ++panel) {
                    draw_digit(image, panel / 10, panel * 8, 1);
                    draw_digit(image, panel % 10, panel * 8 + 4, 1);
                    image.set(panel * 8, 31, frame::Level::kHighlight);  // Bottom-left corner.
                }
                break;
            }
            for (int bx = 0; bx < life::width() / 8; ++bx) {
                for (int by = 0; by < life::kHeight / 8; ++by) {
                    const int x0 = bx * 8;
                    const int y0 = by * 8;
                    const int chip = panel_map::map_cell(board.panel(), x0, y0 + 7).chip;
                    draw_digit(image, chip / 10, x0, y0 + 1);
                    draw_digit(image, chip % 10, x0 + 4, y0 + 1);
                }
            }
            break;
        case Pattern::kColumnSweep: {
            const int x = static_cast<int>((now_ms / 100) % life::width());
            for (int y = 0; y < life::kHeight; ++y) {
                image.set(x, y, frame::Level::kBright);
            }
            break;
        }
        case Pattern::kRowSweep: {
            const int y = static_cast<int>((now_ms / 150) % life::kHeight);
            for (int x = 0; x < life::width(); ++x) {
                image.set(x, y, frame::Level::kBright);
            }
            break;
        }
        case Pattern::kAllOn:
            image.fill(board.ws2812() ? frame::Level::kHighlight : frame::Level::kBright);
            break;
        case Pattern::kLevels:
            for (int y = 0; y < life::kHeight; ++y) {
                const frame::Level level = y < 10   ? frame::Level::kBright
                                           : y < 21 ? frame::Level::kNormal
                                                    : frame::Level::kDim;
                if (y == 10 || y == 21) {
                    continue;
                }
                for (int x = 0; x < life::width(); ++x) {
                    image.set(x, y, level);
                }
            }
            source = colour::Source::kRain;  // WS2812B: a rainbow, at each level.
            break;
        case Pattern::kCount:
            break;
    }
    board.show(image, source, now_ms);
}

// WS2812B bring-up buttons: Start/Select change the pattern, L/R the brightness, and X, Y, A, B
// and Up the panel layout. The firmware saves each change.
template <typename ChangePattern>
void ws2812_button(Board& board, settings::Settings& s, game::Button button, uint32_t now_ms,
                   ChangePattern& change_pattern) {
    pixel_map::PixelConfig& p = board.pixels();
    switch (button) {
        case game::Button::kStart:
            change_pattern(1, now_ms);
            return;
        case game::Button::kSelect:
            change_pattern(-1, now_ms);
            return;
        case game::Button::kL:
        case game::Button::kR: {
            int brightness = static_cast<int>(s.brightness);
            brightness += button == game::Button::kR ? 5 : -5;
            s.brightness =
                static_cast<uint32_t>(brightness < 5 ? 5 : (brightness > 100 ? 100 : brightness));
            ESP_LOGI(kTag, "brightness %lu %%, last frame approximately %lu mA",
                     static_cast<unsigned long>(s.brightness),
                     static_cast<unsigned long>(board.last_current_ma()));
            break;
        }
        case game::Button::kX:
            p.rows = !p.rows;
            break;
        case game::Button::kY:
            p.serpentine = !p.serpentine;
            break;
        case game::Button::kA:
            p.start = static_cast<pixel_map::Start>((static_cast<int>(p.start) + 1) % 4);
            break;
        case game::Button::kB:
            p.zigzag = !p.zigzag;
            break;
        case game::Button::kUp:
            p.reverse_ring = !p.reverse_ring;
            break;
        default:
            return;
    }
    s.pixels = p;
    board.apply(s);
    log_pixels(board.pixels());
    if (settings::save(s) == ESP_OK) {
        ESP_LOGI(kTag, "WS2812B layout and brightness saved");
    }
}

}  // namespace

void run_bringup(Board& board, settings::Settings& s) {
    gamepad_input::UsbGamepad gamepad;
    gamepad_input::UsbGamepadConfig pad_config;
    pad_config.log_reports = true;
    if (gamepad.start(pad_config) != ESP_OK) {
        ESP_LOGW(kTag, "USB host start failed. The test patterns change automatically.");
    }

    mode_button::ModeButton button;
#if CONFIG_LIFE_MODE_BUTTON
    button.start(CONFIG_LIFE_PIN_MODE_BUTTON);
#endif

    Pattern pattern = Pattern::kChipWalk;
    bool auto_advance = CONFIG_LIFE_BRINGUP_AUTO_ADVANCE_S > 0;
    uint32_t pattern_start_ms = 0;
    ESP_LOGI(kTag,
             "bring-up firmware. Start: next pattern, Select: previous pattern, L/R: "
             "intensity, X/Y/A/B/Up/Down: panel layout options.");
    ESP_LOGI(kTag, "pattern: %s", describe(pattern));
    if (board.ws2812()) {
        ESP_LOGI(kTag,
                 "WS2812B: L/R brightness, X rows/columns, Y serpentine, A start corner, "
                 "B zigzag, Up ring direction.");
        log_pixels(board.pixels());
    } else {
        log_panel(board.panel());
    }

    const auto change_pattern = [&](int step, uint32_t now_ms) {
        const int count = static_cast<int>(Pattern::kCount);
        pattern = static_cast<Pattern>((static_cast<int>(pattern) + step + count) % count);
        pattern_start_ms = now_ms;
        ESP_LOGI(kTag, "pattern: %s", describe(pattern));
    };

    while (true) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        gamepad_input::ButtonEvent event;
        while (gamepad.next_event(event, 0)) {
            if (!event.pressed) {
                continue;
            }
            auto_advance = false;
            if (board.ws2812()) {
                ws2812_button(board, s, event.button, now_ms, change_pattern);
                continue;
            }
            panel_map::PanelConfig& p = board.panel();
            bool layout_changed = true;
            switch (event.button) {
                case game::Button::kStart:
                    change_pattern(1, now_ms);
                    layout_changed = false;
                    break;
                case game::Button::kSelect:
                    change_pattern(-1, now_ms);
                    layout_changed = false;
                    break;
                case game::Button::kL:
                case game::Button::kR: {
                    int intensity = board.intensity();
                    intensity += event.button == game::Button::kR ? 1 : -1;
                    intensity = intensity < 0 ? 0 : (intensity > 15 ? 15 : intensity);
                    board.set_intensity(static_cast<uint8_t>(intensity));
                    ESP_LOGI(kTag, "intensity %d (CONFIG_LIFE_INTENSITY)", intensity);
                    layout_changed = false;
                    break;
                }
                case game::Button::kX:
                    p.block_transpose = !p.block_transpose;
                    break;
                case game::Button::kY:
                    p.block_flip_x = !p.block_flip_x;
                    break;
                case game::Button::kA:
                    p.block_flip_y = !p.block_flip_y;
                    break;
                case game::Button::kB:
                    p.zigzag = !p.zigzag;
                    break;
                case game::Button::kUp:
                    p.reverse_ring = !p.reverse_ring;
                    break;
                case game::Button::kDown:
                    p.flip_boards = !p.flip_boards;
                    break;
                default:
                    layout_changed = false;
                    break;
            }
            if (layout_changed) {
                log_panel(p);
            }
            if (event.button != game::Button::kStart && event.button != game::Button::kSelect) {
                // Save the layout and the intensity, so the game firmware uses them too.
                s.panel = p;
                s.intensity = board.intensity();
                if (settings::save(s) == ESP_OK) {
                    ESP_LOGI(kTag, "panel layout and intensity saved");
                }
            }
        }
        if (button.poll(now_ms)) {
            ESP_LOGI(kTag, "mode button pressed");
        }
        if (auto_advance &&
            now_ms - pattern_start_ms >= CONFIG_LIFE_BRINGUP_AUTO_ADVANCE_S * 1000u) {
            change_pattern(1, now_ms);
        }
        render(board, pattern, now_ms);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
