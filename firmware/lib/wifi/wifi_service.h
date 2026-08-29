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
    WIFI_SRV_AP_EVENT_START,
    WIFI_SRV_STA_EVENT_START,
    
    WIFI_SRV_EVENT_STOP,
    WIFI_SRV_EVENT_CONNECT,
    WIFI_SRV_EVENT_DISCONNECT,

    //station event
    WIFI_SRV_EVENT_STA_STARTED,     /* Driver started, ready for connect     */
    WIFI_SRV_EVENT_STA_CONNECTED,   /* L2 link established                   */
    WIFI_SRV_EVENT_STA_GOT_IP,      /* IP assigned (L3 ready)                */
    WIFI_SRV_EVENT_STA_DISCONNECTED,/* Link lost / failed                    */

    //timeout connect with wifi (station mode)
    WIFI_SRV_EVENT_CONNECT_TIMEOUT, /* No IP within timeout window           */

    //flash credential store/load events (async, non-blocking)
    CREDENTIAL_STORE_EVENT,         /* Save wifi creds to NVS flash          */
    CREDENTIAL_LOAD_EVENT,          /* Load wifi creds from NVS flash        */
} wifi_srv_event_t;

/**
 * @brief Main message carried by the unified queue.
 */
typedef struct {
    wifi_srv_event_t event;
    void* data;
} wifi_srv_msg_t;

esp_err_t wifi_srv_init(wifi_manager_t* mgr);
esp_err_t wifi_srv_start(void);
esp_err_t wifi_srv_post_event(wifi_srv_event_t event, void* data);
esp_err_t wifi_srv_set_credentials(const char* ssid, const char* pass);
esp_err_t wifi_srv_switch_mode(wifi_mode_t mode);
esp_err_t wifi_srv_switch_mode_with_creds(wifi_mode_t mode, wifi_credentials_t* creds);
esp_err_t wifi_srv_stop(void);
esp_err_t wifi_srv_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_SERVICE_H */