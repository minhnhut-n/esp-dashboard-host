/**
 * file name: event_bus.c
 * brief: async event bus built on top of the ESP-IDF esp_event framework.
 * author: minhnhut.n
 */

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_event.h"
#include "event_bus.h"

static const char* EVTAG = "EVENT-BUS";

/* ------------------- Event bases (definitions) ------------- */
ESP_EVENT_DEFINE_BASE(WIFI_APP_EVENT);
ESP_EVENT_DEFINE_BASE(HTTP_APP_EVENT);

/* ------------------- Registry entry -------------------------------- */
typedef struct {
    esp_event_base_t base;
    int32_t          id;
    event_bus_handler_t handler;   /* NULL = not subscribed */
} event_bus_reg_entry_t;

static event_bus_reg_entry_t s_registry[EVENT_BUS_MAX];
static bool s_loop_created = false;
static void event_bus_map_to_base_id(event_type_t type, esp_event_base_t* base, int32_t* id) {
    switch (type) {
    /* wifi events -> WIFI_APP_EVENT */
    case WIFI_GOT_IP:         *base = WIFI_APP_EVENT; *id = WIFI_APP_EVENT_GOT_IP;         break;
    case WIFI_AP_CONNECTED:   *base = WIFI_APP_EVENT; *id = WIFI_APP_EVENT_AP_CONNECTED;   break;
    case WIFI_STA_CONNECTED:  *base = WIFI_APP_EVENT; *id = WIFI_APP_EVENT_STA_CONNECTED;  break;
    case WIFI_DISCONNECT:     *base = WIFI_APP_EVENT; *id = WIFI_APP_EVENT_DISCONNECT;     break;

    /* http / control events -> HTTP_APP_EVENT */
    case HTTP_REQ_CHANGE_CREADS:   *base = HTTP_APP_EVENT; *id = HTTP_APP_EVENT_REQ_CHANGE_CREDS;   break;
    case HTTP_REQ_CHANGE_WF_MODE:  *base = HTTP_APP_EVENT; *id = HTTP_APP_EVENT_REQ_CHANGE_WF_MODE; break;
    case HTTP_CONTROL_DEV:         *base = HTTP_APP_EVENT; *id = HTTP_APP_EVENT_CONTROL_DEVICE;     break;
    case HTTP_GET_DATA:            *base = HTTP_APP_EVENT; *id = HTTP_APP_EVENT_GET_DATA;           break;
    case HTTP_EXIT:                *base = HTTP_APP_EVENT; *id = HTTP_APP_EVENT_EXIT;               break;

    default:
        *base = NULL;
        *id   = -1;
        break;
    }
}

static event_bus_handler_t event_bus_find_handler(esp_event_base_t base, int32_t id) {
    for (int i = 0; i < EVENT_BUS_MAX; i++) {
        if (s_registry[i].base == base && s_registry[i].id == id) {
            return s_registry[i].handler;
        }
    }
    return NULL;
}

/* ------------------- Dispatcher (runs on the esp_event task) ------- */
static void event_bus_dispatch(void* arg, esp_event_base_t base, int32_t id, void* data) {
    (void)arg;

    event_bus_handler_t handler = event_bus_find_handler(base, id);
    if (handler == NULL) {
        ESP_LOGW(EVTAG, "no handler for (base=%s, id=%" PRId32 ")", base, id);
        return;
    }

    void** payload_ptr = (void**)data;
    // call handler
    handler(base, id, payload_ptr != NULL ? *payload_ptr : NULL);
}

/* ------------------- API -------------------------------------------- */
esp_err_t event_bus_init(void) {
    if (s_loop_created) {
        ESP_LOGI(EVTAG, "Already init event bus!");
        return ESP_OK;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(EVTAG, "default event loop already exists");
    } else if (err != ESP_OK) {
        ESP_LOGE(EVTAG, "create default event loop failed: %s", esp_err_to_name(err));
        return err;
    } else {
        s_loop_created = true;
    }

    err = esp_event_handler_register(WIFI_APP_EVENT, ESP_EVENT_ANY_ID, event_bus_dispatch, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(EVTAG, "register WIFI_APP_EVENT handler failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_register(HTTP_APP_EVENT, ESP_EVENT_ANY_ID, event_bus_dispatch, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(EVTAG, "register HTTP_APP_EVENT handler failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(EVTAG, "event_bus initialized");
    return ESP_OK;
}

esp_err_t event_bus_post(event_type_t type, void* data) {
    if (type >= EVENT_BUS_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_event_base_t base;
    int32_t id;
    event_bus_map_to_base_id(type, &base, &id);
    if (base == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_event_post(base, id, &data, sizeof(data), 0);
}

esp_err_t event_bus_subscribe(event_type_t type, event_bus_handler_t handler) {
    if (type >= EVENT_BUS_MAX || handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    event_bus_map_to_base_id(type, &s_registry[type].base, &s_registry[type].id);
    if (s_registry[type].base == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_registry[type].handler = handler;

    ESP_LOGI(EVTAG, "subscribed handler for event %d", type);
    return ESP_OK;
}

esp_err_t event_bus_unsubscribe(event_type_t type) {
    if (type >= EVENT_BUS_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    s_registry[type].handler = NULL;
    ESP_LOGI(EVTAG, "unsubscribed handler for event %d", type);
    return ESP_OK;
}

esp_err_t event_bus_deinit(void) {
    esp_err_t err = esp_event_handler_unregister(HTTP_APP_EVENT, ESP_EVENT_ANY_ID, event_bus_dispatch);
    if (err != ESP_OK) {
        ESP_LOGW(EVTAG, "unregister HTTP_APP_EVENT handler failed: %s", esp_err_to_name(err));
    }

    err = esp_event_handler_unregister(WIFI_APP_EVENT, ESP_EVENT_ANY_ID, event_bus_dispatch);
    if (err != ESP_OK) {
        ESP_LOGW(EVTAG, "unregister WIFI_APP_EVENT handler failed: %s", esp_err_to_name(err));
    }

    memset(s_registry, 0, sizeof(s_registry));

    if (s_loop_created) {
        esp_err_t del_err = esp_event_loop_delete_default();
        if (del_err != ESP_OK && del_err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(EVTAG, "delete default loop failed: %s", esp_err_to_name(del_err));
        }
        s_loop_created = false;
    }

    ESP_LOGI(EVTAG, "event_bus deinitialized");
    return ESP_OK;
}