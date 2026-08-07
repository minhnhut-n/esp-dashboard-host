/**
 * @file http_service.h
 * @brief HTTP Service - Initializes HTTP server, handles requests, and manages responses
 * Responsibilities:
 * - Initialize HTTP server
 * - Create task if needed
 * - Register ESP-IDF httpd handler
 * - Receive request
 * - Parse request
 * - Convert request to command/event
 * - Call http-manager
 * - Send response
 */

#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "http_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP Service handle (opaque type)
 */
typedef struct http_service http_service_t;

/**
 * @brief Initialize HTTP Service
 * 
 * @param manager Pointer to http_manager_t instance
 * @return Pointer to http_service_t instance, or NULL on failure
 */
http_service_t* http_service_init(http_manager_t* manager);

/**
 * @brief Start HTTP Service
 * 
 * @param service Pointer to http_service_t instance
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_service_start(http_service_t* service);

/**
 * @brief Stop HTTP Service
 * 
 * @param service Pointer to http_service_t instance
 */
void http_service_stop(http_service_t* service);

/**
 * @brief Register HTTP handler for URI
 * 
 * @param service Pointer to http_service_t instance
 * @param uri URI path to register
 * @param method HTTP method (GET, POST, etc.)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_service_register_handler(http_service_t* service, const char* uri, const char* method);

/**
 * @brief Get HTTP Service status
 * 
 * @param service Pointer to http_service_t instance
 * @param running Pointer to store running status
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_service_get_status(http_service_t* service, uint8_t* running);

/**
 * @brief Deinitialize HTTP Service and free resources
 * 
 * @param service Pointer to http_service_t instance
 */
void http_service_deinit(http_service_t* service);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVICE_H */