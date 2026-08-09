/**
 * file name: wifi_service.h
 * brief: async wifi service - unified queue + dispatcher task for wifi events.
 * author: minhnhut.n
 */

#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "esp_err.h"
#include "wifi_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    //access point event
    WIFI_SRV_EVENT_START,           /* Bring up wifi driver stack            */
    WIFI_SRV_EVENT_STOP,            /* Tear down wifi driver stack           */
    WIFI_SRV_EVENT_CONNECT,         /* data: wifi_credentials_t* (copied)    */
    WIFI_SRV_EVENT_DISCONNECT,      /* Disconnect from AP                    */

    //station event
    WIFI_SRV_EVENT_STA_STARTED,     /* Driver started, ready for connect     */
    WIFI_SRV_EVENT_STA_CONNECTED,   /* L2 link established                   */
    WIFI_SRV_EVENT_STA_GOT_IP,      /* IP assigned (L3 ready)                */
    WIFI_SRV_EVENT_STA_DISCONNECTED,/* Link lost / failed                    */

    //timeout connect with wifi (station mode)
    WIFI_SRV_EVENT_CONNECT_TIMEOUT, /* No IP within timeout window           */
} wifi_srv_event_t;

/**
 * @brief Main message carried by the unified queue.
 */
typedef struct {
    wifi_srv_event_t event;
    void* data;
} wifi_srv_msg_t;

/**
 * @brief Initialize wifi service (create unified queue + handler).
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t wifi_srv_init(wifi_manager_t* mgr);

/**
 * @brief Start the dispatcher wifi_service_task.
 *
 * The task blocks on xQueueReceive(portMAX_DELAY) and only forwards
 * events to wifi_handler_process_event().
 *
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t wifi_srv_start(void);

/**
 * @brief Post a command/event from any context (app task, timer callback).
 *
 * Non-blocking. LIghtway method to public message (only applied on queue of
 * this package)
 * 
 * @param data  Optional payload; credentials for WIFI_SRV_EVENT_CONNECT.
 * @return ESP_OK on success, ESP_ERR_NO_MEM if queue is full.
 */
esp_err_t wifi_srv_post_event(wifi_srv_event_t event, void* data);

/**
 * @brief Stop the dispatcher task.
 *
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t wifi_srv_stop(void);

/**
 * @brief Deinitialize wifi service and free the unified queue.
 *
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t wifi_srv_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_SERVICE_H */