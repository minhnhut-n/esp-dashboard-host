/**
 * @file wifi_manager.h
 * @brief WiFi Manager - Manages WiFi configuration, status, credentials, and mode
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi configuration structure
 * Contains: config, status, credential, mode, mutex
 */
typedef struct {
    char sta_ssid[32];           /**< Station SSID */
    char sta_password[64];       /**< Station password */
    char ap_ssid[32];            /**< Access Point SSID */
    char ap_password[64];        /**< Access Point password */
    wifi_mode_t mode;            /**< WiFi mode (STA/AP/APSTA) */
    uint8_t sta_max_retry;       /**< Maximum retry count for STA connection */
    uint8_t ap_channel;          /**< AP channel */
    uint8_t ap_max_connections;  /**< Maximum AP connections */
} wifi_config_data_t;

/**
 * @brief WiFi status structure
 */
typedef struct {
    wifi_mode_t mode;            /**< Current WiFi mode */
    wifi_ap_record_t ap_info;    /**< AP information (when in STA mode) */
    int8_t rssi;                 /**< Signal strength */
    uint8_t sta_connected;       /**< Station connection status */
    uint8_t ap_started;          /**< AP started status */
    uint8_t ap_clients_count;    /**< Number of connected AP clients */
} wifi_status_t;

/**
 * @brief WiFi Manager handle (opaque type)
 */
typedef struct wifi_manager wifi_manager_t;

/**
 * @brief Initialize WiFi Manager
 * 
 * @return Pointer to wifi_manager_t instance, or NULL on failure
 */
wifi_manager_t* wifi_manager_init(void);

/**
 * @brief Get WiFi configuration
 * 
 * @param manager Pointer to wifi_manager_t instance
 * @param config Pointer to store configuration data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_get_config(wifi_manager_t* manager, wifi_config_data_t* config);

/**
 * @brief Set WiFi configuration
 * 
 * @param manager Pointer to wifi_manager_t instance
 * @param config Pointer to configuration data to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_set_config(wifi_manager_t* manager, const wifi_config_data_t* config);

/**
 * @brief Get WiFi status
 * 
 * @param manager Pointer to wifi_manager_t instance
 * @param status Pointer to store status data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_get_status(wifi_manager_t* manager, wifi_status_t* status);

/**
 * @brief Set WiFi mode
 * 
 * @param manager Pointer to wifi_manager_t instance
 * @param mode WiFi mode to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_set_mode(wifi_manager_t* manager, wifi_mode_t mode);

/**
 * @brief Deinitialize WiFi Manager and free resources
 * 
 * @param manager Pointer to wifi_manager_t instance
 */
void wifi_manager_deinit(wifi_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */