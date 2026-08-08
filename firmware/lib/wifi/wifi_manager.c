#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "string.h"
#include <stdlib.h>

static const char* wifi_log_tag = "WIFI_TAG";

struct wifi_manager {
    wifi_credentials_t creds;
    wifi_mode_t mode;
    wifi_config_t config;
    SemaphoreHandle_t mutex;
};

wifi_manager_t* wifi_manager_create(void) {
    wifi_manager_t* mgr = calloc(1, sizeof(wifi_manager_t));
    if (!mgr) return NULL;

    mgr->mutex = xSemaphoreCreateMutex();
    if (mgr->mutex == NULL) {
        ESP_LOGI(wifi_log_tag, "fail to create semaphore");
        free(mgr);
        return NULL;
    }
    return mgr;
}

// esp_err_t wifi_manager_destroy(wifi_manager_t obj);
// esp_err_t wifi_manager_init(wifi_manager_t obj);
// esp_err_t wifi_manager_get_config(wifi_manager_t obj, wifi_mode_t mode, wifi_config_t* config);
// esp_err_t wifi_manager_set_config(wifi_manager_t obj, wifi_mode_t mode, const wifi_config_t* config);
// esp_err_t wifi_manager_set_mode(wifi_manager_t obj, wifi_mode_t mode);
// wifi_mode_t wifi_manager_get_mode(wifi_manager_t obj);