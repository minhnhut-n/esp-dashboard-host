/**
 * @file event_bus.h
 * @brief Event Bus - Isolated event system for inter-component communication
 * Provides: event definition, event post, event subscribe interface
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Event ID type
 */
typedef uint32_t event_id_t;

/**
 * @brief Event data pointer
 */
typedef void* event_data_t;

/**
 * @brief Event callback function type
 * 
 * @param event_id Event ID
 * @param data Event data pointer
 * @param user_data User data pointer passed during subscription
 */
typedef void (*event_callback_t)(event_id_t event_id, event_data_t data, void* user_data);

/**
 * @brief Event Bus handle (opaque type)
 */
typedef struct event_bus event_bus_t;

/**
 * @brief Initialize Event Bus
 * 
 * @return Pointer to event_bus_t instance, or NULL on failure
 */
event_bus_t* event_bus_init(void);

/**
 * @brief Post event to Event Bus
 * 
 * @param bus Pointer to event_bus_t instance
 * @param event_id Event ID to post
 * @param data Event data pointer (can be NULL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t event_bus_post(event_bus_t* bus, event_id_t event_id, event_data_t data);

/**
 * @brief Subscribe to event
 * 
 * @param bus Pointer to event_bus_t instance
 * @param event_id Event ID to subscribe to
 * @param callback Callback function to be called when event is posted
 * @param user_data User data pointer to be passed to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t event_bus_subscribe(event_bus_t* bus, event_id_t event_id, 
                             event_callback_t callback, void* user_data);

/**
 * @brief Unsubscribe from event
 * 
 * @param bus Pointer to event_bus_t instance
 * @param event_id Event ID to unsubscribe from
 * @param callback Callback function to remove
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t event_bus_unsubscribe(event_bus_t* bus, event_id_t event_id, 
                               event_callback_t callback);

/**
 * @brief Deinitialize Event Bus and free resources
 * 
 * @param bus Pointer to event_bus_t instance
 */
void event_bus_deinit(event_bus_t* bus);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_BUS_H */