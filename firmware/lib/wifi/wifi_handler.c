/**
 * file name: wifi_handler.c
 * brief: minimal wifi handler - start driver with AP mode as default.
 * author: minnhut.n
 *
 * Basic scope:
 *   WIFI_SRV_EVENT_START  -> init netif + esp_wifi, start as AP
 *   WIFI_SRV_EVENT_STOP   -> stop driver
 *   WIFI_SRV_EVENT_CONNECT-> not supported yet (STA comes later)
 *
 * Async design:
 *   wifi_handler_start_driver() only spawns wifi_driver_task and returns
 *   immediately, so wifi_service_task is never blocked by the long
 *   esp_wifi_init()/esp_wifi_start() sequence. The driver task performs
 *   the blocking init and updates the FSM state when done.
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi_handler.h"
#include "wifi_manager.h"
#include "storage_manager.h"

static const char* TAG = "wifi_handler";

#define WIFI_HANDLER_DEFAULT_AP_MAXCON 4
#define WIFI_HANDLER_DEFAULT_AP_CHANNEL 1

// need 8kb for large data struct
#define WIFI_HANDLER_DRIVER_TASK_STACK 8192
#define WIFI_HANDLER_DRIVER_TASK_PRIO  5

// 4kb for data storage asynchonous
#define WIFI_HANDLER_FLASH_TASK_STACK 4096
#define WIFI_HANDLER_FLASH_TASK_PRIO  4

typedef struct wifi_handler {
    wifi_manager_t* mgr;
    wifi_fsm_state_t state;
    TaskHandle_t driver_task;   /* non-NULL while driver init is in progress */
    bool wifi_inited;           /* esp_wifi_init done (only once per boot)   */
} wifi_handler_t;

/* singleton pattern */
static wifi_handler_t* s_handler = NULL;

/* Event handler: arg -> wifi_handler_t* (the singleton), event_data -> payload.
   Wired for both WIFI_EVENT and IP_EVENT so state stays in sync. */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    wifi_handler_t* handler = (wifi_handler_t*)arg;
    if (handler == NULL) {
        return;
    }

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA interface started");
            break;
        case WIFI_EVENT_AP_START:
            handler->state = WIFI_FSM_STATE_RUNNING;
            ESP_LOGI(TAG, "AP started");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            handler->state = WIFI_FSM_STATE_CONNECTED;
            ESP_LOGI(TAG, "STA connected");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (event_data != NULL) {
                wifi_event_sta_disconnected_t* dis =
                    (wifi_event_sta_disconnected_t*)event_data;
                ESP_LOGI(TAG, "STA disconnected, reason=%d", dis->reason);
            } else {
                ESP_LOGI(TAG, "STA disconnected");
            }
            handler->state = WIFI_FSM_STATE_CONNECTING;
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            if (event_data != NULL) {
                wifi_event_ap_staconnected_t* info =
                    (wifi_event_ap_staconnected_t*)event_data;
                ESP_LOGI(TAG, "station connected to AP (aid=%d)", info->aid);
            } else {
                ESP_LOGI(TAG, "station connected to AP");
            }
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            if (event_data != NULL) {
                wifi_event_ap_stadisconnected_t* info =
                    (wifi_event_ap_stadisconnected_t*)event_data;
                ESP_LOGI(TAG, "station disconnected from AP (aid=%d)", info->aid);
            } else {
                ESP_LOGI(TAG, "station disconnected from AP");
            }
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (event_data != NULL) {
            ip_event_got_ip_t* ip_info = (ip_event_got_ip_t*)event_data;
            handler->state = WIFI_FSM_STATE_CONNECTED;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ip_info->ip_info.ip));
        }
    }
}

static void wifi_start_with_mode(void* arg) {
    // type case with manager for this task
    wifi_manager_t* mgr = (wifi_manager_t*)arg;
    
    // manager is not init
    if (mgr == NULL) {
        ESP_LOGE(TAG, "driver task: manager is NULL");
        s_handler->state = WIFI_FSM_STATE_POWER_OFF;
        s_handler->driver_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    wifi_mode_t mode = wifi_manager_get_mode(mgr);
    esp_err_t err = ESP_OK;

    if (!s_handler->wifi_inited) {
        esp_netif_init();

        err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "create default event loop failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }

        esp_netif_create_default_wifi_ap();
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        err = esp_wifi_init(&cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                   wifi_event_handler, s_handler);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   wifi_event_handler, s_handler);

        s_handler->wifi_inited = true;
    }

    // state machine monitor
    /* is_running -> stop -> restart with another mode */
    if (s_handler->state != WIFI_FSM_STATE_POWER_OFF) {
        ESP_LOGI(TAG, "stopping current driver before switching mode");
        err = esp_wifi_stop();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_stop (switch) failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        ESP_LOGI(TAG, "previous driver stopped, switching to mode %d", mode);
        s_handler->state = WIFI_FSM_STATE_POWER_OFF;
    }

    /* Get credentials from the manager (single source of truth) */
    wifi_credentials_t creds = wifi_manager_get_credentials(mgr);

    if (mode == WIFI_MODE_AP) 
    {
        wifi_config_t ap_config;
        memset(&ap_config, 0, sizeof(ap_config));
        snprintf((char*)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s",
                 creds.ssid);
        ap_config.ap.max_connection = WIFI_HANDLER_DEFAULT_AP_MAXCON;
        ap_config.ap.channel        = WIFI_HANDLER_DEFAULT_AP_CHANNEL;
        ap_config.ap.authmode       = WIFI_AUTH_OPEN; /* open network for testing */

        /* For open networks the password must be empty (ESP-IDF validates this) */
        if (ap_config.ap.authmode == WIFI_AUTH_OPEN) {
            ap_config.ap.password[0] = '\0';
        } else {
            snprintf((char*)ap_config.ap.password, sizeof(ap_config.ap.password), "%s", creds.pass);
        }

        err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }

        err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        err = esp_wifi_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        s_handler->state = WIFI_FSM_STATE_RUNNING;
        ESP_LOGI(TAG, "AP mode started (ssid=%s)", creds.ssid);

    } 
    else
    {
        /* Build STA config from the manager credentials */
        wifi_config_t sta_config;
        memset(&sta_config, 0, sizeof(sta_config));
        snprintf((char*)sta_config.sta.ssid, sizeof(sta_config.sta.ssid), "%s", creds.ssid);
        snprintf((char*)sta_config.sta.password, sizeof(sta_config.sta.password), "%s", creds.pass);
        sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

        ESP_LOGI(TAG, "STA: set_mode");
        err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        ESP_LOGI(TAG, "STA: set_config");
        /* wifi_interface_t: WIFI_IF_STA=0, NOT WIFI_MODE_STA=1 */
        err = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        ESP_LOGI(TAG, "STA: start");
        err = esp_wifi_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_POWER_OFF;
            s_handler->driver_task = NULL;
            vTaskDelete(NULL);
            return;
        }

        s_handler->state = WIFI_FSM_STATE_CONNECTING;
        ESP_LOGI(TAG, "STA: connect");
        err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
            s_handler->state = WIFI_FSM_STATE_IDLE;
        }
        ESP_LOGI(TAG, "STA mode connecting to %s", creds.ssid);
    }

    s_handler->driver_task = NULL;
    vTaskDelete(NULL);
}

/* ------------------- Flash credential tasks (non-blocking) --------------- */
static void wifi_flash_store_task(void* arg) {
    wifi_credentials_t* creds = (wifi_credentials_t*)arg;
    if (creds == NULL) {
        ESP_LOGE(TAG, "flash store: creds is NULL");
        vTaskDelete(NULL);
        return;
    }

    esp_err_t err = storage_manager_save_wifi_creds((const char*)creds->ssid,
                                                    (const char*)creds->pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "flash store failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "flash store success (ssid=%s)", creds->ssid);
    }

    free(creds);       /* heap copy owned by this task */
    vTaskDelete(NULL); /* task ends itself */
}

static void wifi_flash_load_task(void* arg) {
    (void)arg;

    char ssid[MAX_SSID_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};

    esp_err_t err = storage_manager_load_wifi_creds(ssid, sizeof(ssid),
                                                    pass, sizeof(pass));
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "no stored credentials found in flash");
        } else {
            ESP_LOGE(TAG, "flash load failed: %s", esp_err_to_name(err));
        }
        vTaskDelete(NULL);
        return;
    }

    if (s_handler != NULL && s_handler->mgr != NULL) {
        wifi_manager_set_credentials(s_handler->mgr,
                                     (uint8_t*)ssid,
                                     (uint8_t*)pass);
        ESP_LOGI(TAG, "flash load success (ssid=%s)", ssid);
    }

    vTaskDelete(NULL); /* task ends itself */
}

static esp_err_t wifi_handler_spawn_flash_task(TaskFunction_t task_fn,
                                               void* arg) {
    TaskHandle_t task = NULL;
    BaseType_t ret = xTaskCreate(task_fn, "wifi_flash_task",
                                 WIFI_HANDLER_FLASH_TASK_STACK, arg,
                                 WIFI_HANDLER_FLASH_TASK_PRIO, &task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "failed to create wifi_flash_task");
        /* caller must free arg if heap-owned (e.g. creds) */
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* ------------------- Driver task (existing) ------------------------------ */

// create task on event
static esp_err_t wifi_handler_start_driver(void) {
    if (s_handler->driver_task != NULL) {
        ESP_LOGW(TAG, "driver task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(wifi_start_with_mode, "wifi_driver_task",
                                 WIFI_HANDLER_DRIVER_TASK_STACK,
                                 s_handler->mgr,
                                 WIFI_HANDLER_DRIVER_TASK_PRIO,
                                 &s_handler->driver_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "failed to create wifi_driver_task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t wifi_handler_process_event(wifi_srv_event_t event, void* data) {
    switch (event) {

    case WIFI_SRV_AP_EVENT_START:
    case WIFI_SRV_STA_EVENT_START:
        return wifi_handler_start_driver();

    case WIFI_SRV_EVENT_STOP:
        if (s_handler->state == WIFI_FSM_STATE_POWER_OFF) {
            return ESP_OK;
        }
        esp_err_t err = esp_wifi_stop();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_stop failed: %s", esp_err_to_name(err));
            return err;
        }
        s_handler->state = WIFI_FSM_STATE_POWER_OFF;
        return ESP_OK;

    
    case WIFI_SRV_EVENT_CONNECT:
        /* data is heap-allocated wifi_credentials_t from wifi_service */
        if (data == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
        wifi_credentials_t* creds = (wifi_credentials_t*)data;
        wifi_manager_set_credentials(s_handler->mgr,
                                     (uint8_t*)creds->ssid,
                                     (uint8_t*)creds->pass);
        free(data);

        /* If the driver is already running in STA mode, apply new creds
           and reconnect. If it is OFF, the user should post STA_START. */
        if (s_handler->state == WIFI_FSM_STATE_CONNECTING ||
            s_handler->state == WIFI_FSM_STATE_CONNECTED) {
            esp_wifi_disconnect();
            esp_wifi_connect();
            s_handler->state = WIFI_FSM_STATE_CONNECTING;
        }
        return ESP_OK;


    case CREDENTIAL_STORE_EVENT:
        if (data == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
        esp_err_t spawn_err = wifi_handler_spawn_flash_task(wifi_flash_store_task, data);
        if (spawn_err != ESP_OK) {
            /* task spawn failed: release the heap copy we own */
            free(data);
        }
        return spawn_err;

    case CREDENTIAL_LOAD_EVENT:
        return wifi_handler_spawn_flash_task(wifi_flash_load_task, NULL);

    default:
        ESP_LOGW(TAG, "event %d not handled", event);
        return ESP_OK;
    }
}

esp_err_t wifi_handler_init(wifi_manager_t* mgr) {
    if (mgr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_handler != NULL) {
        return ESP_OK;
    }

    s_handler = calloc(1, sizeof(wifi_handler_t));
    if (s_handler == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_handler->mgr         = mgr;
    s_handler->state       = WIFI_FSM_STATE_POWER_OFF;
    s_handler->driver_task = NULL;
    s_handler->wifi_inited = false;
    return ESP_OK;
}

wifi_fsm_state_t wifi_handler_get_state(void) {
    if (s_handler == NULL) {
        return WIFI_FSM_STATE_POWER_OFF;
    }
    return s_handler->state;
}

esp_err_t wifi_handler_deinit(void) {
    if (s_handler == NULL) {
        return ESP_OK;
    }

    free(s_handler);
    s_handler = NULL;
    return ESP_OK;
}
// public api
esp_err_t wifi_flash_store_creds(void) {
    if (s_handler == NULL || s_handler->mgr == NULL) {
        ESP_LOGE(TAG, "wifi handler not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_credentials_t creds = wifi_manager_get_credentials(s_handler->mgr);

    wifi_credentials_t* heap_creds = malloc(sizeof(wifi_credentials_t));
    if (heap_creds == NULL) {
        ESP_LOGE(TAG, "flash store: malloc failed");
        return ESP_ERR_NO_MEM;
    }
    memcpy(heap_creds, &creds, sizeof(wifi_credentials_t));

    return wifi_handler_spawn_flash_task(wifi_flash_store_task, heap_creds);
}

esp_err_t wifi_flash_load_creds(void) {
    if (s_handler == NULL) {
        ESP_LOGE(TAG, "wifi handler not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    return wifi_handler_spawn_flash_task(wifi_flash_load_task, NULL);
}
