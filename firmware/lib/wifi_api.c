#include "wifi_api.h"

int g_wifi_mode;
const char* g_wifi_tag = "ESP_WIFI_C";

esp_err_t wifi_init_general(void) {
    // for storing config
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // raise another change
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //network interface
    ret = esp_netif_init();
    ret = esp_event_loop_create_default(); // system event (async)
    esp_netif_create_default_wifi_ap(); // dynamic allocate for ap mode config

    //wifi init
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&wifi_config);

    //ap config
    g_wifi_mode = WIFI_MODE_AP;
    wifi_config_t config = {
        .ap = {
            .ssid = ESP_INTERNAL_SSID,
            .ssid_len = strlen(ESP_INTERNAL_SSID),
            .password = ESP_INTERNAL_PASS, 
            .max_connection = ESP_INTERNAL_STAM,
            .channel = ESP_INTERNAL_CHAN,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    
    if (strlen(ESP_INTERNAL_PASS) == 0) {
        config.ap.authmode = WIFI_AUTH_OPEN;
    }

    //softAP and apply config
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    ret = esp_wifi_set_config(WIFI_IF_AP, &config);

    //start wifi
    ret = esp_wifi_start();
    ESP_LOGI(g_wifi_tag, "esp wifi start ap ready!");
    return ret;
}

int wifi_station_mode(int* param) {
    return 0;
}

int wifi_ap_mode(int* param) {
    return 0;
}

int wifi_event_handler(int (*func)(int, int)) {
    return 0;
}