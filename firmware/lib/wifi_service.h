#ifndef _WIFI_SERVICE_H_
#define _WIFI_SERVICE_H_

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "event_bus.h"

// wifi_mode_t
extern int g_wifi_mode;
extern const char* g_wifi_tag;

#define ESP_INTERNAL_SSID "ESP_ALEX"
#define ESP_INTERNAL_PASS "nhut12345"
#define ESP_INTERNAL_CHAN 7
#define ESP_INTERNAL_STAM 1

esp_err_t wifi_init_general(void);
esp_err_t wifi_station_mode(wifi_creds_data_t* creds);
esp_err_t wifi_ap_mode(int* param);

#endif