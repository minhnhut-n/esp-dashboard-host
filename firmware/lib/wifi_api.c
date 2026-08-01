#include "wifi_api.h"
#include "event_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"

int g_wifi_mode;
const char* g_wifi_tag = "ESP_WIFI_C";

static void esp_wifi_event_handler(void* arg, esp_event_base_t e_base, int32_t e_id, void* e_data) {
    if (e_base == WIFI_EVENT) {
        switch (e_id) {
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(g_wifi_tag, "wifi SCAN done!");
                event_bus_post_wifi_scan_done();
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(g_wifi_tag, "wifi as station START!");
                event_bus_post_wifi_sta_start();
                break;
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(g_wifi_tag, "wifi as station STOP!");
                event_bus_post_wifi_sta_stop();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(g_wifi_tag, "wifi as station CONNECTED!");
                event_bus_post_wifi_connected();
                break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)e_data;
                ESP_LOGW(g_wifi_tag, "wifi as station DISCONNECTED! reason=%d", event->reason);
                event_bus_post_wifi_disconnected();
                if (g_wifi_mode == WIFI_MODE_STA) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    esp_wifi_connect();
                }
                break;
            }

            case WIFI_EVENT_AP_START:
                ESP_LOGI(g_wifi_tag, "wifi as AP START!");
                event_bus_post_wifi_ap_start();
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(g_wifi_tag, "wifi as AP STOP!");
                event_bus_post_wifi_ap_stop();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(g_wifi_tag, "wifi as AP CONNECTED!");
                event_bus_post_wifi_ap_sta_connected();
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(g_wifi_tag, "wifi as AP DISCONNECTED!");
                event_bus_post_wifi_ap_sta_disconnected();
                break;

            default:
                break;
        }
        return;
    }
    if (e_base == IP_EVENT) {
        switch (e_id) {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(g_wifi_tag, "wifi station got IP!");
                event_bus_post_wifi_got_ip();
                break;
            case IP_EVENT_STA_LOST_IP:
                ESP_LOGI(g_wifi_tag, "wifi station lost IP!");
                event_bus_post_wifi_lost_ip();
                break;
            case IP_EVENT_TX_RX:
                ESP_LOGI(g_wifi_tag, "wifi on TRANSMISSION!");
                break;
            default:
                break;
        }
        return;
    }
}


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

    // for later station mode switch
    esp_netif_create_default_wifi_sta();

    //esp_event_handler_t là một con trỏ hàm (*void)
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &esp_wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &esp_wifi_event_handler, NULL, NULL);

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
    ESP_LOGI(g_wifi_tag, "esp wifi start ap READY!");
    return ret;
}

esp_err_t wifi_station_mode(wifi_creds_data_t* creds) {
    if (g_wifi_mode == WIFI_MODE_STA) return ESP_OK;
    if (creds == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t ret;
    ret = esp_wifi_stop();

    g_wifi_mode = WIFI_MODE_STA;
    wifi_config_t config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_OPEN
        },
    };
    snprintf((char*) config.sta.ssid, sizeof(config.sta.ssid), "%s", creds->ssid);
    snprintf((char*) config.sta.password, sizeof(config.sta.password), "%s", creds->pass);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    ret = esp_wifi_set_config(WIFI_IF_STA, &config);

    ret = esp_wifi_start();
    ESP_LOGI(g_wifi_tag, "Switch mode to Wifi STA!");

    vTaskDelay(pdMS_TO_TICKS(1000));
    ret = esp_wifi_connect();
    ESP_LOGI(g_wifi_tag, "Connecting to AP...");
    return ret;
}

esp_err_t wifi_ap_mode(int* param) {
    if (g_wifi_mode == WIFI_MODE_AP) return ESP_OK;

    esp_err_t ret;
    ret = esp_wifi_stop();

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

    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    ret = esp_wifi_set_config(WIFI_IF_AP, &config);

    ret = esp_wifi_start();
    ESP_LOGI(g_wifi_tag, "Switch mode to Wifi AP!");
    ESP_LOGI(g_wifi_tag, "Waiting for Client Connect...");
    return ret;
}
