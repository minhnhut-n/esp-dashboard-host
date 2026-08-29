/**
 * file name: wifi_service.c
 * brief: async wifi service - unified queue + dispatcher task.
 * author: minhnhut.n
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "wifi_service.h"
#include "wifi_handler.h"
#include "storage_manager.h"
#include "event_bus.h"
#include "comp_decomp_data.h"

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
    QueueHandle_t  unified_queue;
    TaskHandle_t   task_handle;
    bool           started;
    wifi_manager_t* mgr;
} wifi_srv_ctx_t;

// singleton
static wifi_srv_ctx_t wifi_srv_ctx = {
    .unified_queue = NULL,
    .task_handle   = NULL,
    .started       = false,
    .mgr           = NULL,
};

// not public
static void wifi_service_task(void* arg) {
    (void)arg; //suppress unused parameter warning

    ESP_LOGI(TAG, "wifi_service_task started");
    for (;;) {
        wifi_srv_msg_t msg;
        BaseType_t ret = xQueueReceive(wifi_srv_ctx.unified_queue, &msg, portMAX_DELAY);
        if (ret != pdTRUE) {
            continue;
        }

        esp_err_t err = wifi_handler_process_event(msg.event, msg.data);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "handler rejected event %d: %s", msg.event, esp_err_to_name(err));
        }
    }
}

static void wifi_srv_mode_change_handler(esp_event_base_t base, int32_t id, void* data) {
    (void)base;
    (void)id;

    if (data == NULL) {
        ESP_LOGW(TAG, "mode change event without payload");
        return;
    }

    char* payload = (char*)data;

    char mode_str[8] = {0};
    char ssid[MAX_SSID_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};
    cmpds_decompress_get(payload, "mode", mode_str, sizeof(mode_str));
    cmpds_decompress_get(payload, "ssid", ssid, sizeof(ssid));
    cmpds_decompress_get(payload, "pass", pass, sizeof(pass));
    free(payload);

    wifi_mode_t mode;
    if (strcmp(mode_str, "AP") == 0) {
        mode = WIFI_MODE_AP;
    } else if (strcmp(mode_str, "STA") == 0) {
        mode = WIFI_MODE_STA;
    } else {
        ESP_LOGW(TAG, "unsupported mode '%s'", mode_str);
        return;
    }
    
    if (mode == WIFI_MODE_STA) {
        if (ssid[0] != '\0' && pass[0] != '\0') {
            esp_err_t creds_err = wifi_manager_set_credentials(wifi_srv_ctx.mgr,
                                                               (uint8_t*)ssid,
                                                               (uint8_t*)pass);
            if (creds_err != ESP_OK) {
                ESP_LOGW(TAG, "failed to set credentials: %s", esp_err_to_name(creds_err));
            }

            wifi_credentials_t creds;
            memset(&creds, 0, sizeof(creds));
            snprintf((char*)creds.ssid, sizeof(creds.ssid), "%s", ssid);
            snprintf((char*)creds.pass, sizeof(creds.pass), "%s", pass);
            esp_err_t store_err = wifi_srv_post_event(CREDENTIAL_STORE_EVENT, &creds);
            if (store_err != ESP_OK) {
                ESP_LOGW(TAG, "persist creds failed: %s", esp_err_to_name(store_err));
            }
        }
    }
    else {
        ESP_LOGI(TAG, "AP switch request: fixed AP defaults, request creds ignored");
    }

    ESP_LOGI(TAG, "switching wifi mode to %d", (int)mode);
    esp_err_t err = wifi_srv_switch_mode(mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_srv_switch_mode failed: %s", esp_err_to_name(err));
    }
}

/* create a queue for internal message communication in wifi_service
automatically trigger init in wifi_handler when the configuration is set */
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
    wifi_srv_ctx.mgr = mgr;

    // bridge http and wifi
    esp_err_t sub_err = event_bus_subscribe(HTTP_REQ_CHANGE_WF_MODE, wifi_srv_mode_change_handler);
    if (sub_err != ESP_OK) {
        ESP_LOGW(TAG, "subscribe HTTP_REQ_CHANGE_WF_MODE failed: %s", esp_err_to_name(sub_err));
    }

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

esp_err_t wifi_srv_post_event(wifi_srv_event_t event, void* data) {
    if (wifi_srv_ctx.unified_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_srv_msg_t msg = {
        .event = event,
        .data  = data,
    };

    /* Events that can carry a heap-allocated payload get an owned heap copy so
       the caller's buffer stays valid until the wifi task consumes it. START
       events copy creds when the caller provides them; NULL data is untouched. */
    bool payload_copy =
        (event == WIFI_SRV_EVENT_CONNECT ||
         event == CREDENTIAL_STORE_EVENT ||
         ((event == WIFI_SRV_AP_EVENT_START || event == WIFI_SRV_STA_EVENT_START)
          && data != NULL));

    if (payload_copy) {
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

    BaseType_t ret = xQueueSend(wifi_srv_ctx.unified_queue, &msg, 0);
    if (ret != pdTRUE) {
        ESP_LOGW(TAG, "unified queue full, dropping event %d", event);
        if (payload_copy) {
            free(msg.data);
        }
        return ESP_ERR_NO_MEM;
    }

    // if free memory "msg.data" here with success, event can not use this data to post to wifi_handler
    // free/ deallocation moved to "wifi_handler_update_credentials()" under it data is used.

    return ESP_OK;
}

esp_err_t wifi_srv_set_credentials(const char* ssid, const char* pass) {
    if (wifi_srv_ctx.mgr == NULL || ssid == NULL || pass == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_credentials_t padded;
    memset(&padded, 0, sizeof(padded));
    snprintf((char*)padded.ssid, sizeof(padded.ssid), "%s", ssid);
    snprintf((char*)padded.pass, sizeof(padded.pass), "%s", pass);

    esp_err_t err = wifi_manager_set_credentials(wifi_srv_ctx.mgr,
                                                 padded.ssid, padded.pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "update manager credentials failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t wifi_srv_switch_mode(wifi_mode_t mode) {
    if (wifi_srv_ctx.unified_queue == NULL || wifi_srv_ctx.mgr == NULL) {
        ESP_LOGE(TAG, "call wifi_srv_init first");
        return ESP_ERR_INVALID_STATE;
    }

    if (mode == wifi_manager_get_mode(wifi_srv_ctx.mgr)) {
        ESP_LOGI(TAG, "already in mode %d", mode);
        return ESP_OK;
    }

    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_STA) {
        ESP_LOGE(TAG, "unsupported mode %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    wifi_manager_set_mode(wifi_srv_ctx.mgr, mode);

    wifi_srv_event_t start_event = (mode == WIFI_MODE_AP)
                                       ? WIFI_SRV_AP_EVENT_START
                                       : WIFI_SRV_STA_EVENT_START;
    return wifi_srv_post_event(start_event, NULL);
}

esp_err_t wifi_srv_switch_mode_with_creds(wifi_mode_t mode, wifi_credentials_t* creds) {
    if (wifi_srv_ctx.unified_queue == NULL || wifi_srv_ctx.mgr == NULL) {
        ESP_LOGE(TAG, "call wifi_srv_init first");
        return ESP_ERR_INVALID_STATE;
    }

    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_STA) {
        ESP_LOGE(TAG, "unsupported mode %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    if (creds != NULL) {
        if (mode != WIFI_MODE_STA) {
            ESP_LOGI(TAG, "AP start: fixed AP defaults, request creds ignored");
        } else {
            ESP_LOGI(TAG, "switch to STA with creds ssid=%s", creds->ssid);
            esp_err_t mgr_err = wifi_manager_set_credentials(wifi_srv_ctx.mgr,
                                                             creds->ssid, creds->pass);
            if (mgr_err != ESP_OK) {
                ESP_LOGE(TAG, "set credentials failed: %s", esp_err_to_name(mgr_err));
                return mgr_err;
            }
        }
    }

    return wifi_srv_switch_mode(mode);
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
            /* START events may carry a heap creds copy too (see post_event) */
            bool owns_data =
                (msg.event == WIFI_SRV_EVENT_CONNECT ||
                 msg.event == CREDENTIAL_STORE_EVENT ||
                 msg.event == WIFI_SRV_AP_EVENT_START ||
                 msg.event == WIFI_SRV_STA_EVENT_START);
            if (owns_data && msg.data != NULL) {
                free(msg.data);
            }
        }
    }

    // kill queue
    if (wifi_srv_ctx.unified_queue != NULL) {
        vQueueDelete(wifi_srv_ctx.unified_queue);
        wifi_srv_ctx.unified_queue = NULL;
    }

    event_bus_unsubscribe(HTTP_REQ_CHANGE_WF_MODE);

    return wifi_handler_deinit();
}
