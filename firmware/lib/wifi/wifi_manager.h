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

typedef enum {
    WIFI_MODE_AP,
    WIFI_MODE_STA,
    WIFI_MODE_NONE
} wifi_manager_mode_t;

typedef struct {
    char ssid[MAX_SSID_LEN];
    char pass[MAX_PASS_LEN];
} wifi_credentials_t;

typedef struct {
    wifi_credentials_t creds;
    wifi_manager_mode_t mode;
    wifi_config_t config;
} wifi_config_custom_t;

//methods
esp_err_t 

#endif // WIFI_MANAGER_H