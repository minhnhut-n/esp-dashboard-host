/**
 * @file event_bus.c
 * @brief Event Bus implementation - Isolated event system for inter-component communication
 */

#include "event_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "event_bus";

/**
 * @brief Event subscriber structure
 */
typedef struct event_subscriber {
    event_id_t event_id;              /**< Event ID */
    event_callback_t callback;        /**< Callback function */
    void* user_data;                  /**< User data pointer */
    struct event_subscriber* next;    /**< Next subscriber in list */
} event_subscriber_t;

/**
 * @brief Event Bus internal structure
 */
struct event_bus {
    event_subscriber_t* subscribers;  /**< Linked list of subscribers */
    SemaphoreHandle_t mutex;          /**< Mutex for thread-safe access */
    uint32_t max_subscribers;         /**< Maximum number of subscribers */
};

#define MAX_SUBSCRIBERS_PER_EVENT 10

event_bus_t* event_bus_init(void) {
    event_bus_t* bus = calloc(1, sizeof(event_bus_t));
    if (!bus) {
        ESP_LOGE(TAG, "Failed to allocate memory for Event Bus");
        return NULL;
    }
    
    /* Create mutex for thread-safe access */
    bus->mutex = xSemaphoreCreateMutex();
    if (!bus->mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(bus);
        return NULL;
    }
    
    bus->subscribers = NULL;
    bus->max_subscribers = MAX_SUBSCRIBERS_PER_EVENT;
    
    ESP_LOGI(TAG, "Event Bus initialized");
    return bus;
}

esp_err_t event_bus_post(event_bus_t* bus, event_id_t event_id, event_data_t data) {
    if (!bus) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Find all subscribers for this event */
    event_subscriber_t* sub = bus->subscribers;
    uint32_t callback_count = 0;
    
    while (sub) {
        if (sub->event_id == event_id) {
            /* Call callback (release mutex during callback to avoid deadlock) */
            xSemaphoreGive(bus->mutex);
            
            ESP_LOGD(TAG, "Posting event 0x%x to subscriber %d", event_id, callback_count);
            sub->callback(event_id, data, sub->user_data);
            callback_count++;
            
            /* Re-acquire mutex */
            if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
                ESP_LOGE(TAG, "Failed to re-acquire mutex");
                return ESP_ERR_TIMEOUT;
            }
        }
        sub = sub->next;
    }
    
    xSemaphoreGive(bus->mutex);
    
    ESP_LOGD(TAG, "Event 0x%x posted to %d subscribers", event_id, callback_count);
    return ESP_OK;
}

esp_err_t event_bus_subscribe(event_bus_t* bus, event_id_t event_id, 
                             event_callback_t callback, void* user_data) {
    if (!bus || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Count subscribers for this event */
    uint32_t count = 0;
    event_subscriber_t* sub = bus->subscribers;
    while (sub) {
        if (sub->event_id == event_id) {
            count++;
        }
        sub = sub->next;
    }
    
    if (count >= bus->max_subscribers) {
        ESP_LOGE(TAG, "Maximum subscribers reached for event 0x%x", event_id);
        xSemaphoreGive(bus->mutex);
        return ESP_ERR_NO_MEM;
    }
    
    /* Create new subscriber */
    event_subscriber_t* new_sub = calloc(1, sizeof(event_subscriber_t));
    if (!new_sub) {
        ESP_LOGE(TAG, "Failed to allocate memory for subscriber");
        xSemaphoreGive(bus->mutex);
        return ESP_ERR_NO_MEM;
    }
    
    new_sub->event_id = event_id;
    new_sub->callback = callback;
    new_sub->user_data = user_data;
    new_sub->next = bus->subscribers;
    bus->subscribers = new_sub;
    
    xSemaphoreGive(bus->mutex);
    
    ESP_LOGI(TAG, "Subscribed to event 0x%x", event_id);
    return ESP_OK;
}

esp_err_t event_bus_unsubscribe(event_bus_t* bus, event_id_t event_id, 
                               event_callback_t callback) {
    if (!bus || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Find and remove subscriber */
    event_subscriber_t** current = &bus->subscribers;
    while (*current) {
        event_subscriber_t* sub = *current;
        if (sub->event_id == event_id && sub->callback == callback) {
            *current = sub->next;
            free(sub);
            
            xSemaphoreGive(bus->mutex);
            ESP_LOGI(TAG, "Unsubscribed from event 0x%x", event_id);
            return ESP_OK;
        }
        current = &sub->next;
    }
    
    xSemaphoreGive(bus->mutex);
    ESP_LOGW(TAG, "Subscriber not found for event 0x%x", event_id);
    return ESP_ERR_NOT_FOUND;
}

void event_bus_deinit(event_bus_t* bus) {
    if (!bus) {
        return;
    }
    
    /* Free all subscribers */
    event_subscriber_t* sub = bus->subscribers;
    while (sub) {
        event_subscriber_t* next = sub->next;
        free(sub);
        sub = next;
    }
    
    /* Delete mutex */
    if (bus->mutex) {
        vSemaphoreDelete(bus->mutex);
    }
    
    free(bus);
    ESP_LOGI(TAG, "Event Bus deinitialized");
}