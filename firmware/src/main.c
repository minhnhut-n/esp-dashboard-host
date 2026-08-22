/**
 * file name: main.c
 * brief: bring up Wi-Fi in AP mode (default) and monitor the handler state.
 * author: minnhut.n
 *
 * Test flow:
 *   app_main posts WIFI_SRV_EVENT_START (non-blocking)
 *   wifi_service_task -> wifi_handler_process_event -> wifi_handler_start_driver
 *   -> AP mode active (ssid: esp-alex / pass: alex1234)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "wifi_service.h"
#include "wifi_handler.h"

#define WIFI_HANDLER_DEFAULT_AP_SSID   "ESP_ALEX"
#define WIFI_HANDLER_DEFAULT_AP_PASS   "nhut12345"

#define WIFI_HANDLER_DEFAULT_STA_SSID   "Thoai Hanh"
#define WIFI_HANDLER_DEFAULT_STA_PASS   "hanh12345"

static const char* TAG = "main";

static const char* fsm_state_name(wifi_fsm_state_t state) {
    switch (state) {
    case WIFI_FSM_STATE_POWER_OFF:  return "POWER_OFF";
    case WIFI_FSM_STATE_IDLE:       return "IDLE";
    case WIFI_FSM_STATE_RUNNING:    return "RUNNING";
    case WIFI_FSM_STATE_CONNECTING: return "CONNECTING";
    case WIFI_FSM_STATE_CONNECTED:  return "CONNECTED";
    default:                        return "UNKNOWN";
    }
}

/* Reports FSM transitions every 2s so you can see AP come up */
static void status_monitor_task(void* arg) {
    (void)arg;

    wifi_fsm_state_t last = WIFI_FSM_STATE_POWER_OFF;
    for (;;) {
        wifi_fsm_state_t cur = wifi_handler_get_state();
        if (cur != last) {
            ESP_LOGI(TAG, "FSM: %s -> %s", fsm_state_name(last), fsm_state_name(cur));
            last = cur;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main started");

    /* NVS is still required by esp_wifi internally */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return;
    }

    /* RAM state store (retained for future STA support) */
    // Create manager
    wifi_manager_t* mgr = wifi_manager_create();
    if (mgr == NULL) {
        ESP_LOGE(TAG, "wifi_manager_create failed");
        return;
    }

    /* Start the async service: unified queue + wifi_service_task */
    // Warm up service wifi (custom)
    err = wifi_srv_init(mgr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_srv_init failed: %s", esp_err_to_name(err));
        return;
    }
    err = wifi_srv_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_srv_start failed: %s", esp_err_to_name(err));
        return;
    }

    /* Watch the FSM transitions */
    xTaskCreate(status_monitor_task, "status_monitor", 2048, NULL, 3, NULL);

    /* Seed the single manager with AP credentials, then start AP. */
    wifi_manager_set_credentials(mgr,
                                 (uint8_t*)WIFI_HANDLER_DEFAULT_AP_SSID,
                                 (uint8_t*)WIFI_HANDLER_DEFAULT_AP_PASS);
    err = wifi_srv_switch_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "switch to AP failed: %s", esp_err_to_name(err));
    }

    /* Demo: after 5s switch to STA with STA credentials.
       wifi_srv_switch_mode stops the driver, updates the manager mode,
       and posts the matching START event - all argument-driven. */
    vTaskDelay(pdMS_TO_TICKS(1000));

    wifi_manager_set_credentials(mgr,
                                 (uint8_t*)WIFI_HANDLER_DEFAULT_STA_SSID,
                                 (uint8_t*)WIFI_HANDLER_DEFAULT_STA_PASS);
    err = wifi_srv_switch_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "switch to STA failed: %s", esp_err_to_name(err));
    }


    /* app_main returns; wifi_service_task + status_monitor keep running */
}