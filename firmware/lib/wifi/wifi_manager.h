/**
 * file name: wifi_manager.h
 * brief: config, status, credential, mode, and muxtex/semaphore for WiFi manager.
 * author: minhnhut.n
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

typedef struct wifi_manager wifi_manager_t;
typedef struct {
    uint8_t ssid[MAX_SSID_LEN];
    uint8_t pass[MAX_PASS_LEN];
} wifi_credentials_t;

esp_err_t wifi_manager_set_config(wifi_manager_t* mgr, wifi_config_t* config);

wifi_manager_t* wifi_manager_create(void);
esp_err_t wifi_manager_destroy(wifi_manager_t* mgr);

esp_err_t wifi_manager_set_mode(wifi_manager_t* mgr, wifi_mode_t mode);
wifi_mode_t wifi_manager_get_mode(wifi_manager_t* mgr);

esp_err_t wifi_manager_set_credentials(wifi_manager_t* mgr, uint8_t* ssid, uint8_t* pass);
wifi_credentials_t wifi_manager_get_credentials(wifi_manager_t* mgr);

// esp_err_t wifi_manager_apply_config(wifi_manager_t* mgr);
#endif // WIFI_MANAGER_H