/**
 * file name: wifi_service.c
 * brief: async wifi service - unified queue + dispatcher task.
 * author: minhnhut.n
 *
 *                     External Apps / System Events
 *                                  |
 *                                  v (Post command / event)
 *                      wifi_srv.unifiedQueue
 *                                  |
 *                                  v xQueueReceive (portMAX_DELAY)
 *                          wifi_service_task  <-- Only dispatches
 *                                  |
 *                                  v wifi_handler_process_event(mgr, event, data)
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "wifi_service.h"
#include "wifi_handler.h"

static const char* TAG = "wifi_srv";

#define WIFI_SRV_QUEUE_LEN   8
#define WIFI_SRV_TASK_STACK  4096
#define WIFI_SRV_TASK_PRIO   5

/**
 * we need:
 * - a task handle for deinit and init when necessary
 * - a unified queue for sending message on this service
 * - a bool variable for fast-tracking state of this module.
*/
typedef struct {
    QueueHandle_t unified_queue;
    TaskHandle_t   task_handle;
    bool           started;
} wifi_srv_ctx_t;


/**
 * Define a static as internal unique varable
 * (intentional arhitecture design)
 */
static wifi_srv_ctx_t wifi_srv_ctx = {
    .unified_queue = NULL,
    .task_handle   = NULL,
    .started       = false,
};

// not public
static void wifi_service_task(void* arg) {
    (void)arg; //suppress unused parameter warning

    ESP_LOGI(TAG, "wifi_service_task started");

    /**
     * the behavior of module totally depend on current context -> state machine
     * se'mantic meaning, this will run forever
     * while(1) mean make a loop with the condition
     */
    for (;;) {
        wifi_srv_msg_t msg;
        BaseType_t ret = xQueueReceive(wifi_srv_ctx.unified_queue, &msg, portMAX_DELAY);
        if (ret != pdTRUE) {
            continue;
        }

        /* Delegate to FSM: fast (<1ms), triggers async drivers then returns */
        esp_err_t err = wifi_handler_process_event(msg.event, msg.data);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "handler rejected event %d: %s", msg.event, esp_err_to_name(err));
        }
    }
}

/* create a queue for internal message communication in wifi_service
automatically trigger init in wifi_handler when the configuration is set*/
esp_err_t wifi_srv_init(wifi_manager_t* mgr) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (wifi_srv_ctx.unified_queue == NULL) {
        wifi_srv_ctx.unified_queue = xQueueCreate(WIFI_SRV_QUEUE_LEN, sizeof(wifi_srv_msg_t));
        if (wifi_srv_ctx.unified_queue == NULL) {
            ESP_LOGE(TAG, "failed to create unified queue");
            return ESP_ERR_NO_MEM;
        }
    }
    //done in wifi_service init

    return wifi_handler_init(mgr);
}

esp_err_t wifi_srv_start(void) {
    if (wifi_srv_ctx.unified_queue == NULL) {
        ESP_LOGE(TAG, "call wifi_srv_init first");
        return ESP_ERR_INVALID_STATE;
    }
    if (wifi_srv_ctx.started) {
        return ESP_OK;
    }

    /* create a task for receiving message to queue, with handle by internal 
    funtion with high priority (5)*/
    BaseType_t ret = xTaskCreate(wifi_service_task, "wifi_service_task",
                                 WIFI_SRV_TASK_STACK, NULL, WIFI_SRV_TASK_PRIO,
                                 &wifi_srv_ctx.task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "failed to create wifi_service_task");
        return ESP_ERR_NO_MEM;
    }

    wifi_srv_ctx.started = true; //update status
    return ESP_OK;
}

/* for the case, application or service want to notify event to wifi_service
it will use this function for public event to "unified_queue" */
esp_err_t wifi_srv_post_event(wifi_srv_event_t event, void* data) {
    if (wifi_srv_ctx.unified_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_srv_msg_t msg = {
        .event = event,
        .data  = data,
    };

    if (event == WIFI_SRV_EVENT_CONNECT) {
        if (data == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
        wifi_credentials_t* creds = malloc(sizeof(wifi_credentials_t));
        if (creds == NULL) {
            ESP_LOGI(TAG, "Fail on allocation mem for creds");
            return ESP_ERR_NO_MEM;
        }
        memcpy(creds, data, sizeof(wifi_credentials_t));
        msg.data = creds; // data is pointed to heap mem -> free is needed.
    }

    // send message to queue (handled by another task - planning with scheduler)
    // this is internal queue
    BaseType_t ret = xQueueSend(wifi_srv_ctx.unified_queue, &msg, 0);
    // when it fail
    if (ret != pdTRUE) {
        ESP_LOGW(TAG, "unified queue full, dropping event %d", event);
        if (event == WIFI_SRV_EVENT_CONNECT) {
            free(msg.data);
        }
        return ESP_ERR_NO_MEM;
    }

    // if free memory "msg.data" here with success, event can not use this data to post to wifi_handler
    // free/ deallocation moved to "wifi_handler_update_credentials()" under it data is used.

    return ESP_OK;
}

esp_err_t wifi_srv_stop(void) {
    if (wifi_srv_ctx.task_handle == NULL) {
        return ESP_OK;
    }

    // cancel queue task on scheduler (not-kill), safe memory action
    vTaskDelete(wifi_srv_ctx.task_handle);
    wifi_srv_ctx.task_handle = NULL;
    wifi_srv_ctx.started     = false;
    return ESP_OK;
}

esp_err_t wifi_srv_deinit(void) {
    if (wifi_srv_ctx.started) {
        wifi_srv_stop();
    }

    // deallocate before killing queue
    if (wifi_srv_ctx.unified_queue != NULL) {
        wifi_srv_msg_t msg;
        while (xQueueReceive(wifi_srv_ctx.unified_queue, &msg, 0) == pdTRUE) {
            if (msg.event == WIFI_SRV_EVENT_CONNECT && msg.data != NULL) {
                free(msg.data);
            }
        }
    }

    // kill queue
    if (wifi_srv_ctx.unified_queue != NULL) {
        vQueueDelete(wifi_srv_ctx.unified_queue);
        wifi_srv_ctx.unified_queue = NULL;
    }

    return wifi_handler_deinit();
}
