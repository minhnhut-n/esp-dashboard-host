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
    char ssid[MAX_SSID_LEN];
    char pass[MAX_PASS_LEN];
} wifi_credentials_t;

/**
 * name: wifi_manager_init
 * args: 
 * - obj: wanted to init
 */
wifi_manager_t* wifi_manager_create(void);
esp_err_t wifi_manager_destroy(wifi_manager_t obj);
esp_err_t wifi_manager_init(wifi_manager_t obj);
esp_err_t wifi_manager_get_config(wifi_manager_t obj, wifi_mode_t mode, wifi_config_t* config);
esp_err_t wifi_manager_set_config(wifi_manager_t obj, wifi_mode_t mode, const wifi_config_t* config);
esp_err_t wifi_manager_set_mode(wifi_manager_t obj, wifi_mode_t mode);
wifi_mode_t wifi_manager_get_mode(wifi_manager_t obj);

#endif // WIFI_MANAGER_H