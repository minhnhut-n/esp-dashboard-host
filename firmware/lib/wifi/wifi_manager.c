/**
 * @file wifi_manager.c
 * @brief WiFi Manager implementation
 */

#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "wifi_manager";

/**
 * @brief WiFi Manager internal structure
 */
struct wifi_manager {
    wifi_config_data_t config;    /**< WiFi configuration */
    wifi_status_t status;         /**< WiFi status */
    SemaphoreHandle_t mutex;      /**< Mutex for thread-safe access */
    EventGroupHandle_t event_group; /**< Event group for WiFi events */
};

/* Event bits for WiFi events */
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1
#define WIFI_AP_STARTED_BIT   BIT2

static esp_err_t wifi_manager_apply_config(wifi_manager_t* manager) {
    if (!manager) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_mode_t mode = manager->config.mode;
    
    /* Configure WiFi */
    wifi_config_t wifi_config = {0};
    
    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        strncpy((char*)wifi_config.sta.ssid, manager->config.sta_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char*)wifi_config.sta.password, manager->config.sta_password, sizeof(wifi_config.sta.password) - 1);
    }
    
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        strncpy((char*)wifi_config.ap.ssid, manager->config.ap_ssid, sizeof(wifi_config.ap.ssid) - 1);
        strncpy((char*)wifi_config.ap.password, manager->config.ap_password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.channel = manager->config.ap_channel;
        wifi_config.ap.max_connections = manager->config.ap_max_connections;
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_set_mode(mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "WiFi configuration applied successfully");
    return ESP_OK;
}

wifi_manager_t* wifi_manager_init(void) {
    wifi_manager_t* manager = calloc(1, sizeof(wifi_manager_t));
    if (!manager) {
        ESP_LOGE(TAG, "Failed to allocate memory for WiFi manager");
        return NULL;
    }
    
    /* Create mutex for thread-safe access */
    manager->mutex = xSemaphoreCreateMutex();
    if (!manager->mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(manager);
        return NULL;
    }
    
    /* Create event group */
    manager->event_group = xEventGroupCreate();
    if (!manager->event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        vSemaphoreDelete(manager->mutex);
        free(manager);
        return NULL;
    }
    
    /* Initialize WiFi */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    /* Set default configuration */
    memset(&manager->config, 0, sizeof(manager->config));
    manager->config.mode = WIFI_MODE_STA;
    manager->config.sta_max_retry = 5;
    manager->config.ap_channel = 1;
    manager->config.ap_max_connections = 4;
    
    /* Set default status */
    memset(&manager->status, 0, sizeof(manager->status));
    manager->status.mode = WIFI_MODE_STA;
    
    ESP_LOGI(TAG, "WiFi Manager initialized");
    return manager;
}

esp_err_t wifi_manager_get_config(wifi_manager_t* manager, wifi_config_data_t* config) {
    if (!manager || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(manager->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    memcpy(config, &manager->config, sizeof(wifi_config_data_t));
    
    xSemaphoreGive(manager->mutex);
    return ESP_OK;
}

esp_err_t wifi_manager_set_config(wifi_manager_t* manager, const wifi_config_data_t* config) {
    if (!manager || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(manager->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    memcpy(&manager->config, config, sizeof(wifi_config_data_t));
    
    esp_err_t ret = wifi_manager_apply_config(manager);
    
    xSemaphoreGive(manager->mutex);
    return ret;
}

esp_err_t wifi_manager_get_status(wifi_manager_t* manager, wifi_status_t* status) {
    if (!manager || !status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(manager->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Update status from WiFi */
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    manager->status.mode = mode;
    
    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        esp_wifi_sta_get_ap_info(&manager->status.ap_info);
        manager->status.rssi = manager->status.ap_info.rssi;
    }
    
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        manager->status.sta_connected = 1;
    } else {
        manager->status.sta_connected = 0;
    }
    
    memcpy(status, &manager->status, sizeof(wifi_status_t));
    
    xSemaphoreGive(manager->mutex);
    return ESP_OK;
}

esp_err_t wifi_manager_set_mode(wifi_manager_t* manager, wifi_mode_t mode) {
    if (!manager) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP && mode != WIFI_MODE_APSTA) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(manager->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    manager->config.mode = mode;
    esp_err_t ret = wifi_manager_apply_config(manager);
    
    xSemaphoreGive(manager->mutex);
    return ret;
}

void wifi_manager_deinit(wifi_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    /* Stop WiFi */
    esp_wifi_stop();
    esp_wifi_deinit();
    
    /* Clean up resources */
    if (manager->event_group) {
        vEventGroupDelete(manager->event_group);
    }
    
    if (manager->mutex) {
        vSemaphoreDelete(manager->mutex);
    }
    
    free(manager);
    ESP_LOGI(TAG, "WiFi Manager deinitialized");
}