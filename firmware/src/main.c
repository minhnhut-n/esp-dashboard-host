#include "wifi_api.h"
#include "event_bus.h"
#include "http_json_helper.h"

void app_main() {
    event_bus_init();
    wifi_init_general();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    wifi_creds_data_t creds = {
        .ssid = "Thoai Hanh",
        .pass = "hanh12345"
    };
    wifi_station_mode(&creds);

    // Start HTTP server after WiFi is ready
    vTaskDelay(pdMS_TO_TICKS(2000));  // wait for IP
    start_http_server();
}
