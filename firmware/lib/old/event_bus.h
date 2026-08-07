#ifndef _EVENT_BUS_H_
#define _EVENT_BUS_H_

#include <esp_event.h>
#include <string.h>

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 32

// Register the event bases once in the implementation file.
ESP_EVENT_DECLARE_BASE(WIFI_APP_EVENT);
ESP_EVENT_DECLARE_BASE(HTTP_APP_EVENT);

typedef enum {
    WIFI_CONNECTED_EVE,
    WIFI_DISCONNECTED_EVE,
    WIFI_GOT_IP_EVE,
    WIFI_LOST_IP_EVE,
    WIFI_STA_START_EVE,
    WIFI_STA_STOP_EVE,
    WIFI_AP_START_EVE,
    WIFI_AP_STOP_EVE,
    WIFI_AP_STA_CONNECTED_EVE,
    WIFI_AP_STA_DISCONNECTED_EVE,
    WIFI_SCAN_DONE_EVE,
} wifi_event_custom_t;

typedef enum {
    HTTP_EXIT_EVE,
} http_event_custom_t;

typedef struct {
    char ssid[MAX_SSID_LEN];
    char pass[MAX_SSID_LEN];
} wifi_creds_data_t;

// Event bus functions
esp_err_t event_bus_init(void);
void event_bus_post_wifi_event(wifi_event_custom_t event_id, void* event_data, size_t event_data_size);
void event_bus_post_wifi_connected(void);
void event_bus_post_wifi_disconnected(void);
void event_bus_post_wifi_got_ip(void);
void event_bus_post_wifi_lost_ip(void);
void event_bus_post_wifi_sta_start(void);
void event_bus_post_wifi_sta_stop(void);
void event_bus_post_wifi_ap_start(void);
void event_bus_post_wifi_ap_stop(void);
void event_bus_post_wifi_ap_sta_connected(void);
void event_bus_post_wifi_ap_sta_disconnected(void);
void event_bus_post_wifi_scan_done(void);

void event_bus_post_http_event(http_event_custom_t event_id, void* event_data, size_t event_data_size);
void event_bus_post_http_exit(void);

#endif // _EVENT_BUS_H_
