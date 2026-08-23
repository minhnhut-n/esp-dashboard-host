/**
 * file name: wifi_handler.h
 * brief: wifi FSM - state transitions in RAM, triggers async drivers, returns.
 * author: minhnhut.n
 */

#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include "esp_err.h"
#include "wifi_manager.h"
#include "wifi_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_FSM_STATE_POWER_OFF,   /* Driver not initialized                */
    WIFI_FSM_STATE_IDLE,        /* Driver initialized, not connected     */
    WIFI_FSM_STATE_RUNNING,
    WIFI_FSM_STATE_CONNECTING,  /* esp_wifi_connect issued, awaiting link */
    WIFI_FSM_STATE_CONNECTED,   /* L2 connected, awaiting IP (or ready)  */
} wifi_fsm_state_t;

typedef struct wifi_handler wifi_handler_t;

esp_err_t wifi_handler_init(wifi_manager_t* mgr);
esp_err_t wifi_handler_process_event(wifi_srv_event_t event, void* data);
wifi_fsm_state_t wifi_handler_get_state(void);
esp_err_t wifi_handler_deinit(void);

// asychonize nvs flash handler
esp_err_t wifi_flash_store_creds(void);
esp_err_t wifi_flash_load_creds(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_HANDLER_H */