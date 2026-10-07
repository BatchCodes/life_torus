// SPDX-License-Identifier: GPL-3.0-or-later
#include "max7219_chain/max7219_chain.hpp"

#include "esp_check.h"

namespace max7219_chain {

namespace {

constexpr const char* kTag = "max7219_chain";

}  // namespace

esp_err_t Max7219Chain::init(const ChainConfig& config) {
    spi_bus_config_t bus = {};
    bus.mosi_io_num = config.din_gpio;
    bus.miso_io_num = -1;
    bus.sclk_io_num = config.clk_gpio;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = sizeof(buffer_);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(config.host, &bus, SPI_DMA_DISABLED), kTag,
                        "SPI bus init failed");

    spi_device_interface_config_t device = {};
    device.mode = 0;
    device.clock_speed_hz = config.clock_hz;
    device.spics_io_num = config.cs_gpio;
    device.queue_size = 1;
    // The MAX7219 latches on the rising edge of LOAD. Keep CS low a little after the last bit.
    device.cs_ena_posttrans = 2;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(config.host, &device, &device_), kTag,
                        "SPI device add failed");
    return ESP_OK;
}

esp_err_t Max7219Chain::write_digit(uint8_t reg, const uint8_t* values) {
    // The first 16 bits that go out end in the last chip of the chain, so send the last chip
    // first.
    const int chips = panel_map::chips();
    for (int chip = 0; chip < chips; ++chip) {
        const int slot = chips - 1 - chip;
        buffer_[slot * 2] = reg;
        buffer_[slot * 2 + 1] = values[chip];
    }
    spi_transaction_t transaction = {};
    transaction.length = static_cast<size_t>(chips) * 16;
    transaction.tx_buffer = buffer_;
    return spi_device_polling_transmit(device_, &transaction);
}

esp_err_t Max7219Chain::write_all(uint8_t reg, uint8_t value) {
    uint8_t values[panel_map::kMaxChips];
    for (uint8_t& v : values) {
        v = value;
    }
    return write_digit(reg, values);
}

esp_err_t Max7219Chain::configure(uint8_t intensity) {
    ESP_RETURN_ON_ERROR(write_all(kDisplayTest, 0x00), kTag, "display test write failed");
    ESP_RETURN_ON_ERROR(write_all(kDecodeMode, 0x00), kTag, "decode mode write failed");
    ESP_RETURN_ON_ERROR(write_all(kScanLimit, 0x07), kTag, "scan limit write failed");
    ESP_RETURN_ON_ERROR(set_intensity(intensity), kTag, "intensity write failed");
    ESP_RETURN_ON_ERROR(write_all(kShutdown, 0x01), kTag, "shutdown write failed");
    return ESP_OK;
}

esp_err_t Max7219Chain::set_intensity(uint8_t intensity) {
    return write_all(kIntensity, intensity > 15 ? 15 : intensity);
}

esp_err_t Max7219Chain::write(const panel_map::Registers& registers) {
    for (int digit = 0; digit < panel_map::kDigits; ++digit) {
        ESP_RETURN_ON_ERROR(
            write_digit(static_cast<uint8_t>(kDigit0 + digit), registers[digit].data()), kTag,
            "digit write failed");
    }
    return ESP_OK;
}

}  // namespace max7219_chain
