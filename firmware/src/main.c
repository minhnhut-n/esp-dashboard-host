#include "wifi_api.h"
#include "event_bus.h"
#include "http_json_helper.h"
#include "esp_log.h"

const char wifi_task[] = "WIFI_TASK";

esp_err_t http_register_wifi_handler(void) {
    esp_err_t ret = esp_event_handler_instance_register(
        WIFI_APP_EVENT,
        WIFI_AP_STOP_EVE,
        http_wifi_stop_handler,
        NULL,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(g_http_tag, "failed to register AP stop handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_APP_EVENT,
        WIFI_STA_STOP_EVE,
        http_wifi_stop_handler,
        NULL,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(g_http_tag, "failed to register STA stop handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        HTTP_APP_EVENT,
        HTTP_EXIT_EVE,
        http_wifi_stop_handler,
        NULL,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(g_http_tag, "failed to register HTTP EXIT stop: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_APP_EVENT,
        WIFI_STA_START_EVE,
        http_wifi_start_handler,
        NULL,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(g_http_tag, "failed to register STA start handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_APP_EVENT,
        WIFI_AP_START_EVE,
        http_wifi_start_handler,
        NULL,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(g_http_tag, "failed to register AP start handler: %s", esp_err_to_name(ret));
    }

    return ret;
}

void esp_sw_sta_mode(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    char ssid[MAX_SSID_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};
    wifi_creds_data_t creds = {0};

    load_wifi_creds(ssid, sizeof(ssid), pass, sizeof(pass));
    snprintf(creds.ssid, sizeof(creds.ssid), "%s", ssid);
    snprintf(creds.pass, sizeof(creds.pass), "%s", pass);

    ESP_LOGI(g_http_tag, "Switching to STA mode with SSID: %s", creds.ssid);
    wifi_station_mode(&creds);
}

void app_main() {
    event_bus_init();
    esp_event_handler_register(WIFI_APP_EVENT, WIFI_AP_STA_DISCONNECTED_EVE, esp_sw_sta_mode, NULL);
    esp_event_handler_register(HTTP_APP_EVENT, HTTP_EXIT_EVE, esp_sw_sta_mode, NULL);
    //http start with ip which provided by esp32
    http_register_wifi_handler();

    //with AP mode
    wifi_init_general();

}
