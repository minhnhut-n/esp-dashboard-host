/**
 * @file http_manager.h
 * @brief HTTP Manager - Manages HTTP server configuration and provides API only
 * Does not run task or handle event loop
 */

#ifndef HTTP_MANAGER_H
#define HTTP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP server configuration structure
 * Contains: server config, port, ip, endpoint table, json parser, nvs keyname
 */
typedef struct {
    uint16_t port;                 /**< HTTP server port */
    char ip_address[16];           /**< Server IP address */
    uint8_t max_clients;           /**< Maximum concurrent clients */
    uint32_t timeout_ms;           /**< Request timeout in milliseconds */
    char nvs_keyname[32];          /**< NVS key name for storing config */
} http_server_config_t;

/**
 * @brief HTTP endpoint structure
 */
typedef struct {
    char uri[64];                  /**< URI path */
    char method[16];               /**< HTTP method (GET, POST, etc.) */
    void* handler_data;            /**< Handler data pointer */
} http_endpoint_t;

/**
 * @brief HTTP Manager handle (opaque type)
 */
typedef struct http_manager http_manager_t;

/**
 * @brief Initialize HTTP Manager
 * 
 * @return Pointer to http_manager_t instance, or NULL on failure
 */
http_manager_t* http_manager_init(void);

/**
 * @brief Get HTTP server configuration
 * 
 * @param manager Pointer to http_manager_t instance
 * @param config Pointer to store server configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_manager_get_config(http_manager_t* manager, http_server_config_t* config);

/**
 * @brief Register HTTP endpoint
 * 
 * @param manager Pointer to http_manager_t instance
 * @param endpoint Pointer to endpoint configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_manager_register_endpoint(http_manager_t* manager, const http_endpoint_t* endpoint);

/**
 * @brief Send HTTP response
 * 
 * @param manager Pointer to http_manager_t instance
 * @param status_code HTTP status code
 * @param content_type Content type string
 * @param data Response data
 * @param data_len Data length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_manager_send_response(http_manager_t* manager, uint16_t status_code,
                                     const char* content_type, const char* data, size_t data_len);

/**
 * @brief Parse JSON request data
 * 
 * @param manager Pointer to http_manager_t instance
 * @param json_str JSON string to parse
 * @param output Output buffer for parsed data
 * @param output_len Output buffer length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_manager_parse_json(http_manager_t* manager, const char* json_str,
                                  char* output, size_t output_len);

/**
 * @brief Deinitialize HTTP Manager and free resources
 * 
 * @param manager Pointer to http_manager_t instance
 */
void http_manager_deinit(http_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_MANAGER_H */