/**
 * file name: wifi_handler.c
 * brief: minimal wifi handler - start driver with AP mode as default.
 * author: minnhut.n
 *
 * Basic scope:
 *   WIFI_SRV_EVENT_START  -> init netif + esp_wifi, start as AP
 *   WIFI_SRV_EVENT_STOP   -> stop driver
 *   WIFI_SRV_EVENT_CONNECT-> not supported yet (STA comes later)
 */

#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi_handler.h"
#include "wifi_manager.h"

static const char* TAG = "wifi_handler";

#define WIFI_HANDLER_DEFAULT_AP_SSID   "esp-alex"
#define WIFI_HANDLER_DEFAULT_AP_PASS   "alex1234"
#define WIFI_HANDLER_DEFAULT_AP_MAXCON 4
#define WIFI_HANDLER_DEFAULT_AP_CHANNEL 1

typedef struct wifi_handler {
    wifi_manager_t* mgr;
    wifi_fsm_state_t state;
} wifi_handler_t;

/* singleton pattern */
static wifi_handler_t* s_handler = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA interface started");
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "AP started");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station connected to AP");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station disconnected from AP");
            break;
        default:
            break;
        }
    }
}

static esp_err_t wifi_handler_start_driver(void) {
    esp_netif_init();

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "create default event loop failed: %s", esp_err_to_name(err));
        return ESP_ERR_NO_MEM;
    }

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return ESP_ERR_WIFI_NOT_INIT;
    }

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               wifi_event_handler, NULL);

    /* Get credentials from the manager (seeded in main.c) */
    wifi_credentials_t creds = wifi_manager_get_credentials(s_handler->mgr);

    /* Fall back to defaults if manager creds are empty */
    if (creds.ssid[0] == '\0') {
        snprintf((char*)creds.ssid, sizeof(creds.ssid), "%s",
                 WIFI_HANDLER_DEFAULT_AP_SSID);
        snprintf((char*)creds.pass, sizeof(creds.pass), "%s",
                 WIFI_HANDLER_DEFAULT_AP_PASS);
    }

    /* Build AP config from the manager credentials */
    wifi_config_t ap_config;
    memset(&ap_config, 0, sizeof(ap_config));
    snprintf((char*)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s",
             creds.ssid);
    ap_config.ap.max_connection = WIFI_HANDLER_DEFAULT_AP_MAXCON;
    ap_config.ap.channel       = WIFI_HANDLER_DEFAULT_AP_CHANNEL;
    ap_config.ap.authmode      = WIFI_AUTH_OPEN; /* open network for testing */

    /* For open networks the password must be empty (ESP-IDF validates this) */
    if (ap_config.ap.authmode == WIFI_AUTH_OPEN) {
        ap_config.ap.password[0] = '\0';
    } else {
        snprintf((char*)ap_config.ap.password, sizeof(ap_config.ap.password), "%s",
                 creds.pass);
    }

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_set_config(WIFI_MODE_AP, &ap_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return err;
    }

    s_handler->state = WIFI_FSM_STATE_RUNNING;
    ESP_LOGI(TAG, "AP mode started (ssid=%s)", creds.ssid);
    return ESP_OK;
}

esp_err_t wifi_handler_process_event(wifi_srv_event_t event, void* data) {
    switch (event) {

    case WIFI_SRV_EVENT_START:
        if (s_handler->state != WIFI_FSM_STATE_POWER_OFF) {
            ESP_LOGW(TAG, "START ignored - driver already running");
            return ESP_OK;
        }
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
        if (data != NULL) {
            free(data);
        }
        ESP_LOGW(TAG, "CONNECT not supported - AP only for now");
        return ESP_ERR_NOT_SUPPORTED;

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

    s_handler->mgr   = mgr;
    s_handler->state = WIFI_FSM_STATE_POWER_OFF;
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