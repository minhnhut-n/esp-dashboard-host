#include "wifi_api.h"
#include "http_json_helper.h"

void app_main() {
    wifi_init_general();
    vTaskDelay(pdMS_TO_TICKS(3000));
    wifi_station_mode(NULL);

    // Start HTTP server after WiFi is ready
    vTaskDelay(pdMS_TO_TICKS(2000));  // wait for IP
    start_http_server();
}