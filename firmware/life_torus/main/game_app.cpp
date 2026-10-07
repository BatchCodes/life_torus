// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "app_settings.hpp"
#include "board.hpp"
#include "display_modes/mode_manager.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "game/game.hpp"
#include "gamepad_input/usb_gamepad.hpp"
#include "microphone/microphone.hpp"
#include "mode_button/mode_button.hpp"
#include "modes.hpp"
#include "patterns/library.hpp"
#include "patterns/presets.hpp"
#include "phone_link/phone_link.hpp"
#include "sdkconfig.h"
#include "settings/nvs_store.hpp"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t listen_js_start[] asm("_binary_listen_js_start");
extern const uint8_t listen_js_end[] asm("_binary_listen_js_end");

namespace {

constexpr const char* kTag = "game";
constexpr uint32_t kTickMs = 10;
constexpr uint32_t kRenderMs = 20;
constexpr uint32_t kPhoneFrameMs = 100;
constexpr const char* kGameStates[] = {"run", "pause", "new preset", "effect"};

const phone_link::StaticFile kPhoneFiles[] = {
    {"/index.html", "text/html", index_html_start, index_html_end},
    {"/app.js", "text/javascript", app_js_start, app_js_end},
    {"/style.css", "text/css", style_css_start, style_css_end},
    {"/listen.js", "text/javascript", listen_js_start, listen_js_end},
};

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

// Appends "&key=" and the percent-encoded value.
void append_value(char* out, size_t size, const char* key, const char* value) {
    char encoded[3 * display_modes::kMaxTextLength + 1];
    if (phone_link::url_encode(value, encoded, sizeof(encoded)) == 0 && value[0] != '\0') {
        encoded[0] = '\0';
    }
    const size_t used = std::strlen(out);
    std::snprintf(out + used, size - used, "&%s=%s", key, encoded);
}

void send_status(phone_link::PhoneLink& phone, const game::Game& game,
                 const display_modes::ModeManager& modes, bool mic, float bpm) {
    char message[800];
    std::snprintf(message, sizeof(message),
                  "status game=%s&display_mode=%d&generation=%lu&population=%d&preset=%d&speed=%d"
                  "&mic=%d&bpm=%d",
                  kGameStates[static_cast<int>(game.mode())], static_cast<int>(modes.mode()),
                  static_cast<unsigned long>(game.simulation().generation()),
                  game.simulation().current().population(), game.preset_index(), modes.speed(),
                  mic ? 1 : 0, static_cast<int>(bpm + 0.5f));
    append_value(message, sizeof(message), "preset_name", game.preset_name());
    append_value(message, sizeof(message), "shape", game.shape_name());
    append_value(message, sizeof(message), "text", modes.text());
    phone.send_text(message);
}

void send_presets(phone_link::PhoneLink& phone) {
    char message[600] = "presets ";
    bool first = true;
    for (const patterns::Preset& preset : patterns::presets()) {
        char encoded[96];
        if (phone_link::url_encode(preset.name, encoded, sizeof(encoded)) == 0) {
            continue;
        }
        const size_t used = std::strlen(message);
        std::snprintf(message + used, sizeof(message) - used, "%s%s", first ? "" : "&", encoded);
        first = false;
    }
    phone.send_text(message);
}

void send_settings(phone_link::PhoneLink& phone, const settings::Settings& s) {
    char message[settings::kMaxText + 16] = "settings ";
    settings::to_text(s, message + 9, sizeof(message) - 9, false);
    phone.send_text(message);
}

}  // namespace

void run_game(Board& board, settings::Settings& s) {
    gamepad_input::UsbGamepad gamepad;
    gamepad_input::UsbGamepadConfig pad_config;
#if CONFIG_LIFE_LOG_CONTROLLER_REPORTS
    pad_config.log_reports = true;
#endif
    if (gamepad.start(pad_config) != ESP_OK) {
        ESP_LOGW(kTag, "USB host start failed. The display runs with no controller.");
    }

    game::Game game(game_config(s));
    display_modes::ModeManager modes(game, esp_random());
    game.start(now_ms());
    ESP_LOGI(kTag, "started with preset \"%s\"", game.preset_name());

    mode_button::ModeButton button;
#if CONFIG_LIFE_MODE_BUTTON
    button.start(CONFIG_LIFE_PIN_MODE_BUTTON);
#endif

    microphone::Microphone mic;
#if CONFIG_LIFE_MIC
    microphone::MicConfig mic_config;
    mic_config.sck_gpio = CONFIG_LIFE_PIN_MIC_SCK;
    mic_config.ws_gpio = CONFIG_LIFE_PIN_MIC_WS;
    mic_config.sd_gpio = CONFIG_LIFE_PIN_MIC_SD;
    if (mic.start(mic_config) == ESP_OK) {
        mic.set_sensitivity(static_cast<int>(s.beat_sensitivity));
    }
#endif

    phone_link::PhoneLink phone;
    bool phone_on = false;
#if CONFIG_LIFE_WIFI
    phone_link::PhoneLinkConfig phone_config;
    phone_config.ssid = CONFIG_LIFE_WIFI_SSID;
    phone_config.files = kPhoneFiles;
    phone_config.file_count = sizeof(kPhoneFiles) / sizeof(kPhoneFiles[0]);
    phone_on = phone.start(phone_config, s.password) == ESP_OK;
    if (!phone_on) {
        ESP_LOGW(kTag, "Wi-Fi start failed. The phone app is not available.");
    }
#endif

    frame::Image image;
    uint32_t last_render = 0;
    uint32_t last_phone_frame = 0;
    int last_preset = game.preset_index();
    int last_clients = 0;
    TickType_t wake = xTaskGetTickCount();
    while (true) {
        gamepad_input::ButtonEvent event;
        while (gamepad.next_event(event, 0)) {
            if (event.pressed) {
                modes.press(event.button, now_ms());
            } else {
                modes.release(event.button, now_ms());
            }
        }

        phone_link::Command command;
        while (phone_on && phone.next_command(command, 0)) {
            const uint32_t now = now_ms();
            const bool in_game = modes.mode() == display_modes::ModeId::kGameOfLife;
            switch (command.type) {
                case phone_link::CommandType::kPress:
                    modes.press(command.button, now);
                    break;
                case phone_link::CommandType::kRelease:
                    modes.release(command.button, now);
                    break;
                case phone_link::CommandType::kCell:
                    if (in_game) {
                        game.tap_cell(command.a, command.b, now);
                    }
                    break;
                case phone_link::CommandType::kPreset:
                    if (in_game) {
                        game.select_preset(command.a, now);
                    }
                    break;
                case phone_link::CommandType::kMode:
                    if (command.a >= 0 && command.a < display_modes::kModeCount) {
                        modes.set_mode(static_cast<display_modes::ModeId>(command.a), now);
                        ESP_LOGI(kTag, "display mode \"%s\"",
                                 display_modes::mode_name(modes.mode()));
                    }
                    break;
                case phone_link::CommandType::kText:
                    modes.set_text(command.text);
                    break;
                case phone_link::CommandType::kSpeed:
                    modes.set_speed(command.a);
                    break;
                case phone_link::CommandType::kSettings: {
                    settings::Settings changed = s;
                    if (!settings::from_text(command.text, changed)) {
                        ESP_LOGW(kTag, "settings from the phone are not valid");
                        break;
                    }
                    const bool new_password = std::strcmp(changed.password, s.password) != 0;
                    s = changed;
                    board.apply(s);
                    game.set_config(game_config(s));
                    mic.set_sensitivity(static_cast<int>(s.beat_sensitivity));
                    if (new_password) {
                        phone.set_password(s.password);
                    }
                    if (settings::save(s) == ESP_OK) {
                        ESP_LOGI(kTag, "settings saved");
                    }
                    send_settings(phone, s);
                    break;
                }
                case phone_link::CommandType::kGetSettings:
                    send_settings(phone, s);
                    break;
            }
        }

        const uint32_t now = now_ms();
        if (button.poll(now)) {
            modes.next_mode(now);
            ESP_LOGI(kTag, "display mode \"%s\" (button)", display_modes::mode_name(modes.mode()));
        }
        // The microphone beat goes to the display modes. Game of Life ignores it.
        if (mic.take_beat(now) && s.beat_sync) {
            modes.beat(now);
        }
        modes.tick(now);
        if (game.preset_index() != last_preset) {
            last_preset = game.preset_index();
            ESP_LOGI(kTag, "preset \"%s\"", game.preset_name());
        }
        if (now - last_render >= kRenderMs) {
            last_render = now;
            modes.render(image, now);
            board.show(image);
        }
        if (phone_on && phone.client_count() > 0) {
            if (phone.client_count() > last_clients) {
                send_presets(phone);
                send_settings(phone, s);
            }
            if (now - last_phone_frame >= kPhoneFrameMs) {
                last_phone_frame = now;
                phone.send_frame(image);
                send_status(phone, game, modes, mic.present(), mic.bpm());
            }
        }
        last_clients = phone_on ? phone.client_count() : 0;
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(kTickMs));
    }
}
