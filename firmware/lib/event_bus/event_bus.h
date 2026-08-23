/**
 * file name: event_bus.h
 * brief: async event bus built on top of the ESP-IDF esp_event framework.
 * author: minhnhut.n
 */

#ifndef _EVENT_BUS_
#define _EVENT_BUS_

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------- Event bases --------------------------- */
/* Declared here, defined (allocated) in event_bus.c for each module */
ESP_EVENT_DECLARE_BASE(WIFI_APP_EVENT);
ESP_EVENT_DECLARE_BASE(HTTP_APP_EVENT);

/* ------------------- Event IDs per base -------------------- */
typedef enum {
    WIFI_APP_EVENT_GOT_IP,        /* IP assigned (L3 ready)     */
    WIFI_APP_EVENT_AP_CONNECTED,  /* a station joined the AP    */
    WIFI_APP_EVENT_STA_CONNECTED, /* STA link established       */
    WIFI_APP_EVENT_DISCONNECT,    /* link lost / failed         */
} wifi_app_event_id_t;

typedef enum {
    HTTP_APP_EVENT_REQ_CHANGE_CREDS,    /* change wifi credentials     */
    HTTP_APP_EVENT_REQ_CHANGE_WF_MODE,  /* change wifi mode            */
    HTTP_APP_EVENT_CONTROL_DEVICE,      /* control a device            */
    HTTP_APP_EVENT_GET_DATA,            /* request data                */
} http_app_event_id_t;

/* ------------------- Legacy logical event types ------------ */
/* Kept for backward compatibility: each entry maps to one of the
   two bases above. See event_bus_map_to_base_id() in event_bus.c. */
typedef enum {
    /* wifi events -> WIFI_APP_EVENT */
    WIFI_GOT_IP,
    WIFI_AP_CONNECTED,
    WIFI_STA_CONNECTED,
    WIFI_DISCONNECT,

    /* http / control events -> HTTP_APP_EVENT */
    HTTP_REQ_CHANGE_CREADS,
    HTTP_REQ_CHANGE_WF_MODE,
    HTTP_CONTROL_DEV,
    HTTP_GET_DATA,

    EVENT_BUS_MAX,   /* keep last: number of event types */
} event_type_t;

/* ------------------- Handler callback ---------------------- */
// user define
typedef void (*event_bus_handler_t)(esp_event_base_t base, int32_t id, void* data);

/* ------------------- API ----------------------------------- */
esp_err_t event_bus_init(void);
esp_err_t event_bus_post(event_type_t type, void* data);
esp_err_t event_bus_subscribe(event_type_t type, event_bus_handler_t handler);
esp_err_t event_bus_unsubscribe(event_type_t type);
esp_err_t event_bus_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_BUS_ */