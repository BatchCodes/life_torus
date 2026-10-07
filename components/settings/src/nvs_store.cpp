// SPDX-License-Identifier: GPL-3.0-or-later
#include "settings/nvs_store.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace settings {

namespace {

constexpr const char* kTag = "settings";
constexpr const char* kNamespace = "life_torus";
constexpr const char* kKey = "settings";

}  // namespace

esp_err_t nvs_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(kTag, "NVS partition erased (old format or full)");
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), kTag, "NVS erase failed");
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t load(Settings& s) {
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;  // Nothing saved yet.
    }
    ESP_RETURN_ON_ERROR(err, kTag, "NVS open failed");
    char text[kMaxText];
    size_t length = sizeof(text);
    err = nvs_get_str(handle, kKey, text, &length);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(err, kTag, "NVS read failed");
    if (!from_text(text, s)) {
        ESP_LOGW(kTag, "saved settings are not valid, using the defaults");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(kTag, "saved settings loaded");
    return ESP_OK;
}

esp_err_t save(const Settings& s) {
    char text[kMaxText];
    ESP_RETURN_ON_FALSE(to_text(s, text, sizeof(text), true) > 0, ESP_ERR_INVALID_SIZE, kTag,
                        "settings text too long");
    nvs_handle_t handle = 0;
    ESP_RETURN_ON_ERROR(nvs_open(kNamespace, NVS_READWRITE, &handle), kTag, "NVS open failed");
    esp_err_t err = nvs_set_str(handle, kKey, text);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    ESP_RETURN_ON_ERROR(err, kTag, "NVS write failed");
    return ESP_OK;
}

}  // namespace settings
