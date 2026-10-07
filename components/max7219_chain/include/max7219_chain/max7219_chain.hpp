// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "panel_map/panel_map.hpp"

namespace max7219_chain {

struct ChainConfig {
    int din_gpio = -1;
    int clk_gpio = -1;
    int cs_gpio = -1;
    int clock_hz = 2000000;
    spi_host_device_t host = SPI2_HOST;
};

// MAX7219 register addresses (datasheet table 2).
enum Register : uint8_t {
    kNoOp = 0x00,
    kDigit0 = 0x01,
    kDecodeMode = 0x09,
    kIntensity = 0x0A,
    kScanLimit = 0x0B,
    kShutdown = 0x0C,
    kDisplayTest = 0x0F,
};

// The 32 MAX7219 chips of the eight boards as one SPI device. The chips are in a daisy chain, so
// one transfer writes one register in every chip. CS (LOAD) latches the data on its rising edge.
class Max7219Chain {
public:
    esp_err_t init(const ChainConfig& config);

    // Writes the start-up registers: no decode, scan all 8 digits, display test off, normal
    // operation. Call it again at any time to recover from a glitch on the supply or the wires.
    esp_err_t configure(uint8_t intensity);
    // 0 (lowest) to 15 (highest). The same value in every chip.
    esp_err_t set_intensity(uint8_t intensity);
    // Writes the 8 digit registers of all chips.
    esp_err_t write(const panel_map::Registers& registers);

private:
    // Writes `value` to `reg` in every chip.
    esp_err_t write_all(uint8_t reg, uint8_t value);
    // Writes one register in every chip, with a different value for each chip.
    esp_err_t write_digit(uint8_t reg, const uint8_t* values);

    spi_device_handle_t device_ = nullptr;
    uint8_t buffer_[panel_map::kMaxChips * 2] = {};
};

}  // namespace max7219_chain
