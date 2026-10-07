// SPDX-License-Identifier: GPL-3.0-or-later
#include "max7219_chain/refresh.hpp"

#include "esp_check.h"
#include "esp_log.h"

namespace max7219_chain {

namespace {

constexpr const char* kTag = "max7219_refresh";

}  // namespace

esp_err_t Refresh::start(Max7219Chain& chain, const RefreshConfig& config) {
    chain_ = &chain;
    config_ = config;
    if (config_.subframes < 1) {
        config_.subframes = 1;
    }
    if (config_.subframes > kMaxSubframes) {
        config_.subframes = kMaxSubframes;
    }
    intensity_ = config_.intensity;
    lock_ = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(lock_ != nullptr, ESP_ERR_NO_MEM, kTag, "mutex create failed");
    ESP_RETURN_ON_ERROR(chain_->configure(config_.intensity), kTag, "chain configure failed");
    const BaseType_t created = xTaskCreatePinnedToCore(
        task_entry, "max7219", 4096, this, config_.task_priority, nullptr, config_.task_core);
    ESP_RETURN_ON_FALSE(created == pdPASS, ESP_ERR_NO_MEM, kTag, "task create failed");
    return ESP_OK;
}

void Refresh::show(const std::array<panel_map::Registers, kMaxSubframes>& subframes) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    pending_ = subframes;
    has_pending_ = true;
    xSemaphoreGive(lock_);
}

void Refresh::task_entry(void* arg) {
    static_cast<Refresh*>(arg)->run();
}

void Refresh::run() {
    TickType_t wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(config_.subframe_period_ms) > 0
                                  ? pdMS_TO_TICKS(config_.subframe_period_ms)
                                  : 1;
    TickType_t last_reinit = wake;
    int subframe = 0;
    uint8_t written_intensity = intensity_;
    while (true) {
        if (intensity_ != written_intensity) {
            written_intensity = intensity_;
            config_.intensity = written_intensity;
            if (chain_->set_intensity(written_intensity) != ESP_OK) {
                ESP_LOGW(kTag, "intensity write failed");
            }
        }
        if (subframe == 0 && has_pending_) {
            xSemaphoreTake(lock_, portMAX_DELAY);
            active_ = pending_;
            has_pending_ = false;
            xSemaphoreGive(lock_);
        }
        if (config_.reinit_period_ms > 0 &&
            xTaskGetTickCount() - last_reinit >= pdMS_TO_TICKS(config_.reinit_period_ms)) {
            last_reinit = xTaskGetTickCount();
            if (chain_->configure(config_.intensity) != ESP_OK) {
                ESP_LOGW(kTag, "re-init failed");
            }
        }
        if (chain_->write(active_[subframe]) != ESP_OK) {
            ESP_LOGW(kTag, "frame write failed");
        }
        subframe = (subframe + 1) % config_.subframes;
        vTaskDelayUntil(&wake, period);
    }
}

}  // namespace max7219_chain
