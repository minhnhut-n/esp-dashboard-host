#include "event_bus.h"
#include "esp_log.h"

ESP_EVENT_DEFINE_BASE(WIFI_APP_EVENT);

static const char* TAG = "EVENT_BUS";

esp_err_t event_bus_init(void) {
    ESP_LOGI(TAG, "Event bus initialized with base: WIFI_APP_EVENT");
    return ESP_OK;
}

void event_bus_post_wifi_event(wifi_event_custom_t event_id, void* event_data, size_t event_data_size) {
    esp_err_t ret = esp_event_post(WIFI_APP_EVENT, event_id, event_data, event_data_size, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post event %d, error: %s", event_id, esp_err_to_name(ret));
    }
}

void event_bus_post_wifi_connected(void) {
    event_bus_post_wifi_event(WIFI_CONNECTED_EVE, NULL, 0);
}

void event_bus_post_wifi_disconnected(void) {
    event_bus_post_wifi_event(WIFI_DISCONNECTED_EVE, NULL, 0);
}

void event_bus_post_wifi_got_ip(void) {
    event_bus_post_wifi_event(WIFI_GOT_IP_EVE, NULL, 0);
}

void event_bus_post_wifi_lost_ip(void) {
    event_bus_post_wifi_event(WIFI_LOST_IP_EVE, NULL, 0);
}

void event_bus_post_wifi_sta_start(void) {
    event_bus_post_wifi_event(WIFI_STA_START_EVE, NULL, 0);
}

void event_bus_post_wifi_sta_stop(void) {
    event_bus_post_wifi_event(WIFI_STA_STOP_EVE, NULL, 0);
}

void event_bus_post_wifi_ap_start(void) {
    event_bus_post_wifi_event(WIFI_AP_START_EVE, NULL, 0);
}

void event_bus_post_wifi_ap_stop(void) {
    event_bus_post_wifi_event(WIFI_AP_STOP_EVE, NULL, 0);
}

void event_bus_post_wifi_ap_sta_connected(void) {
    event_bus_post_wifi_event(WIFI_AP_STA_CONNECTED_EVE, NULL, 0);
}

void event_bus_post_wifi_ap_sta_disconnected(void) {
    event_bus_post_wifi_event(WIFI_AP_STA_DISCONNECTED_EVE, NULL, 0);
}

void event_bus_post_wifi_scan_done(void) {
    event_bus_post_wifi_event(WIFI_SCAN_DONE_EVE, NULL, 0);
}
