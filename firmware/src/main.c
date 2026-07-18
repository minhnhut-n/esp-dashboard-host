#include "wifi_api.h"

void app_main() {
    wifi_init_general();
    vTaskDelay(pdMS_TO_TICKS(3000));
    wifi_station_mode(NULL);
}