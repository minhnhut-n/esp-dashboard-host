/**
 * @file wifi_service.h
 * @brief WiFi Service - Handles WiFi operations, command queue, and event processing
 */

#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "wifi_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi Service command types
 */
typedef enum {
    WIFI_CMD_SCAN = 0,              /**< Scan for WiFi networks */
    WIFI_CMD_CONNECT,               /**< Connect to WiFi network */
    WIFI_CMD_DISCONNECT,            /**< Disconnect from WiFi */
    WIFI_CMD_START_AP,              /**< Start Access Point */
    WIFI_CMD_STOP_AP,               /**< Stop Access Point */
    WIFI_CMD_GET_STATUS,            /**< Get WiFi status */
    WIFI_CMD_SET_CONFIG,            /**< Set WiFi configuration */
    WIFI_CMD_GET_CONFIG,            /**< Get WiFi configuration */
    WIFI_CMD_SET_MODE,              /**< Set WiFi mode */
    WIFI_CMD_MAX                    /**< Maximum command ID */
} wifi_service_cmd_t;

/**
 * @brief WiFi Service command structure
 */
typedef struct {
    wifi_service_cmd_t cmd;         /**< Command type */
    void* data;                     /**< Command data (optional) */
    size_t data_len;                /**< Data length */
    uint32_t timeout_ms;            /**< Command timeout in milliseconds */
} wifi_service_cmd_msg_t;

/**
 * @brief WiFi Service handle (opaque type)
 * Contains: queue, process command, call wifi manager
 */
typedef struct wifi_service wifi_service_t;

/**
 * @brief Initialize WiFi Service
 * 
 * @param manager Pointer to wifi_manager_t instance
 * @return Pointer to wifi_service_t instance, or NULL on failure
 */
wifi_service_t* wifi_service_init(wifi_manager_t* manager);

/**
 * @brief Start WiFi Service task
 * 
 * @param service Pointer to wifi_service_t instance
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_service_start(wifi_service_t* service);

/**
 * @brief Stop WiFi Service task
 * 
 * @param service Pointer to wifi_service_t instance
 */
void wifi_service_stop(wifi_service_t* service);

/**
 * @brief Send command to WiFi Service queue
 * 
 * @param service Pointer to wifi_service_t instance
 * @param cmd_msg Pointer to command message
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_service_send_command(wifi_service_t* service, const wifi_service_cmd_msg_t* cmd_msg);

/**
 * @brief Get WiFi Service status
 * 
 * @param service Pointer to wifi_service_t instance
 * @param running Pointer to store running status (1=running, 0=stopped)
 * @param queue_count Pointer to store number of commands in queue
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_service_get_status(wifi_service_t* service, uint8_t* running, uint32_t* queue_count);

/**
 * @brief Deinitialize WiFi Service and free resources
 * 
 * @param service Pointer to wifi_service_t instance
 */
void wifi_service_deinit(wifi_service_t* service);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_SERVICE_H */