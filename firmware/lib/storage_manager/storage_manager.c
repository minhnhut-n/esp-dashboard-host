/**
 * @file storage_manager.c
 * @brief Storage Manager - NVS (non-volatile flash) wrapper.
 *
 * Design:
 *  - All NVS operations (open/set/get/commit/erase/close) are serialized
 *    through an internal mutex (binary semaphore). Because flash access is
 *    slow, the caller must perform these operations inside a dedicated
 *    FreeRTOS task so the application dispatcher task is never blocked.
 *
 * @author minnhut.n
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage_manager.h"

static const char* TAG = "storage_mgr";

struct storage_manager {
    SemaphoreHandle_t lock;   /* binar semaphore guarding the NVS resource */
};

/* singleton */
static storage_manager_t* s_manager = NULL;

/**
 * @brief Initialize the NVS flash partition (idempotent).
 */
static esp_err_t storage_nvs_flash_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs_flash_init: erasing and re-initializing");
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    return err;
}

storage_manager_t* storage_manager_init(void) {
    if (s_manager != NULL) {
        return s_manager;  /* idempotent singleton */
    }

    esp_err_t err = storage_nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return NULL;
    }

    storage_manager_t* mgr = calloc(1, sizeof(storage_manager_t));
    if (mgr == NULL) {
        ESP_LOGE(TAG, "alloc storage_manager failed");
        return NULL;
    }

    mgr->lock = xSemaphoreCreateMutex();
    if (mgr->lock == NULL) {
        ESP_LOGE(TAG, "create storage mutex failed");
        free(mgr);
        return NULL;
    }

    s_manager = mgr;
    ESP_LOGI(TAG, "storage_manager initialized (namespace: %s)", NVS_STORE_NAME);
    return s_manager;
}

esp_err_t storage_manager_read(storage_manager_t* manager, const char* key, void* data, size_t* data_len) {
    if (manager == NULL || key == NULL || data == NULL || data_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* lock the flash resource for the whole read sequence */
    if (xSemaphoreTake(manager->lock, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take storage lock");
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORE_NAME, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(read) failed: %s", esp_err_to_name(err));
        xSemaphoreGive(manager->lock);
        return err;
    }

    err = nvs_get_blob(handle, key, data, data_len);
    nvs_close(handle);

    xSemaphoreGive(manager->lock);
    return err;
}

esp_err_t storage_manager_write(storage_manager_t* manager, const char* key, const void* data, size_t data_len) {
    if (manager == NULL || key == NULL || data == NULL || data_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* lock the flash resource for the whole write sequence */
    if (xSemaphoreTake(manager->lock, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take storage lock");
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORE_NAME, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(write) failed: %s", esp_err_to_name(err));
        xSemaphoreGive(manager->lock);
        return err;
    }

    err = nvs_set_blob(handle, key, data, data_len);
    if (err == ESP_OK) {
        //apply changes
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    xSemaphoreGive(manager->lock);
    return err;
}

esp_err_t storage_manager_delete(storage_manager_t* manager, const char* key) {
    if (manager == NULL || key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(manager->lock, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take storage lock");
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORE_NAME, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(delete) failed: %s", esp_err_to_name(err));
        xSemaphoreGive(manager->lock);
        return err;
    }

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    xSemaphoreGive(manager->lock);
    return err;
}

void storage_manager_deinit(storage_manager_t* manager) {
    if (manager == NULL) {
        return;
    }

    if (manager->lock != NULL) {
        vSemaphoreDelete(manager->lock);
    }
    free(manager);

    if (s_manager == manager) {
        s_manager = NULL;
    }
}

/* ------------------- WiFi credential helpers --------------------------- */

esp_err_t storage_manager_save_wifi_creds(const char* ssid, const char* pass) {
    if (s_manager == NULL || ssid == NULL || pass == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    /* write ssid first; if this fails nothing was stored */
    err = storage_manager_write(s_manager, NVS_WIFI_SSID_KEY,
                                ssid, (size_t) NVS_SSID_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save ssid failed: %s", esp_err_to_name(err));
        return err;
    }

    err = storage_manager_write(s_manager, NVS_WIFI_PASS_KEY,
                                pass, (size_t) NVS_PASS_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save pass failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "wifi credentials saved to flash");
    return ESP_OK;
}

esp_err_t storage_manager_load_wifi_creds(char* ssid, size_t ssid_len, char* pass, size_t pass_len) {
    if (s_manager == NULL || ssid == NULL || pass == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t ssid_actual = ssid_len;
    esp_err_t err = storage_manager_read(s_manager, NVS_WIFI_SSID_KEY,
                                         ssid, &ssid_actual);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load ssid failed: %s", esp_err_to_name(err));
        return err;
    }

    size_t pass_actual = pass_len;
    err = storage_manager_read(s_manager, NVS_WIFI_PASS_KEY,
                               pass, &pass_actual);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load pass failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "wifi credentials loaded from flash");
    return ESP_OK;
}

esp_err_t storage_manager_clear_wifi_creds(void) {
    if (s_manager == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = storage_manager_delete(s_manager, NVS_WIFI_SSID_KEY);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "clear ssid failed: %s", esp_err_to_name(err));
        return err;
    }

    err = storage_manager_delete(s_manager, NVS_WIFI_PASS_KEY);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "clear pass failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "wifi credentials cleared from flash");
    return ESP_OK;
}