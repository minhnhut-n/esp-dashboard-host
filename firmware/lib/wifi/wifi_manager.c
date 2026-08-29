#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "string.h"
#include <stdlib.h>

static const char* wifi_log_tag = "WIFI_TAG";

struct wifi_manager {
    wifi_credentials_t creds;   // for STA mode only
    wifi_mode_t mode;
    wifi_config_t config;
    SemaphoreHandle_t mutex;
};

static esp_err_t wifi_manager_creads_update(wifi_manager_t* mgr) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(mgr->config.sta.ssid, mgr->creds.ssid, MAX_SSID_LEN);
    memcpy(mgr->config.sta.password, mgr->creds.pass, MAX_PASS_LEN);

    return ESP_OK;
}

wifi_manager_t* wifi_manager_create(void) {
    wifi_manager_t* mgr = calloc(1, sizeof(wifi_manager_t));
    if (mgr == NULL) {
        return NULL;
    }

    mgr->mutex = xSemaphoreCreateMutex();
    if (mgr->mutex == NULL) {
        ESP_LOGI(wifi_log_tag, "Fail to create semaphore!");
        free(mgr);
        return NULL;
    }

    return mgr;
}

esp_err_t wifi_manager_destroy(wifi_manager_t* mgr) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vSemaphoreDelete(mgr->mutex);
    free(mgr);
    return ESP_OK;
}

esp_err_t wifi_manager_set_mode(wifi_manager_t* mgr, wifi_mode_t mode) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        mgr->mode = mode;
    }
    xSemaphoreGive(mgr->mutex);
    
    return ESP_OK;
}

wifi_mode_t wifi_manager_get_mode(wifi_manager_t* mgr) {
    if (mgr == NULL) {
        ESP_LOGI(wifi_log_tag, "Invalid argument for getting mode!!");
        return WIFI_MODE_NULL;
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        mode = mgr->mode;
    }
    xSemaphoreGive(mgr->mutex);

    return mode;
}

esp_err_t wifi_manager_set_credentials(wifi_manager_t* mgr, uint8_t* ssid, uint8_t* pass) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(mgr->creds.ssid, ssid, MAX_SSID_LEN);
        memcpy(mgr->creds.pass, pass, MAX_PASS_LEN);
    }
    esp_err_t ret = wifi_manager_creads_update(mgr);
    xSemaphoreGive(mgr->mutex);

    return ret;
}

wifi_credentials_t wifi_manager_get_credentials(wifi_manager_t* mgr) {
    wifi_credentials_t cred;
    memset(&cred, 0, sizeof(cred));
    if (mgr == NULL) {
        ESP_LOGI(wifi_log_tag, "Invalid argument for getting credential !!");
        return cred;
    }

    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(cred.ssid, mgr->creds.ssid, MAX_SSID_LEN);
        memcpy(cred.pass, mgr->creds.pass, MAX_PASS_LEN);
    }
    xSemaphoreGive(mgr->mutex);

    return cred;
}

esp_err_t wifi_manager_set_config(wifi_manager_t* mgr, wifi_config_t* config) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&mgr->config, config, sizeof(wifi_config_t));
    }
    xSemaphoreGive(mgr->mutex);

    return ESP_OK;
}

esp_err_t wifi_manager_get_config(wifi_manager_t* mgr, wifi_config_t* config) {
    if (mgr == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(mgr->mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(config, &mgr->config, sizeof(wifi_config_t));
    }
    xSemaphoreGive(mgr->mutex);

    return ESP_OK;
}
