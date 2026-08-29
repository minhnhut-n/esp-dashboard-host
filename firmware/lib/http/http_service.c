/**
 * file name: http_service.c
 * brief: HTTP service layer - async bridge between HTTP, WiFi and storage.
 *
 * The service maps simple commands coming from the http_handler layer to
 * the wifi_service unified queue and the event bus:
 *   - save wifi creds    -> wifi_srv_post_event(CREDENTIAL_STORE_EVENT)
 *                          (flash write is done async by wifi_handler on a
 *                           dedicated flash task) + notify over event bus
 *   - load wifi creds    -> storage_manager_load_wifi_creds (NVS read)
 *   - control a device   -> event_bus HTTP_CONTROL_DEV
 *   - request exit       -> event_bus HTTP_EXIT
 *
 * author: minhnhut.n
 */

#include "http_service.h"

#include "esp_log.h"
#include <string.h>
#include <stdint.h>

#include "wifi_manager.h"
#include "wifi_service.h"
#include "storage_manager.h"
#include "event_bus.h"

static const char* TAG = "HTTP_SERVICE";

esp_err_t http_service_save_wifi_creds(const char* ssid, const char* pass) {
    if (ssid == NULL || pass == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t mgr_err = wifi_srv_set_credentials(ssid, pass);
    if (mgr_err != ESP_OK) {
        ESP_LOGW(TAG, "update manager credentials failed: %s", esp_err_to_name(mgr_err));
    }

    /* clamp into the fixed-size credential bundle used by wifi_handler */
    wifi_credentials_t creds;
    memset(&creds, 0, sizeof(creds));
    snprintf((char*)creds.ssid, sizeof(creds.ssid), "%s", ssid);
    snprintf((char*)creds.pass, sizeof(creds.pass), "%s", pass);

    /* 1) async persist: the wifi service stores it on the flash task */
    esp_err_t err = wifi_srv_post_event(CREDENTIAL_STORE_EVENT, &creds);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "post CREDENTIAL_STORE_EVENT failed: %s; fallback to sync persist",
                 esp_err_to_name(err));
        err = storage_manager_save_wifi_creds(ssid, pass);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "sync persist failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    /* 2) notify the app layer that the credentials changed */
    event_bus_post(HTTP_REQ_CHANGE_CREADS, NULL);

    return ESP_OK;
}

esp_err_t http_service_load_wifi_creds(char* ssid, size_t ssid_size,
                                       char* pass, size_t pass_size) {
    if (ssid == NULL || pass == NULL || ssid_size == 0 || pass_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return storage_manager_load_wifi_creds(ssid, ssid_size, pass, pass_size);
}

esp_err_t http_service_switch_wifi_mode(const char* payload) {
    if (payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "switch wifi mode request (payload=%s)", payload);

    /* Ownership of the heap-allocated compressed payload transfers to the
       event bus -> wifi service subscriber, which frees it after unpacking. */
    return event_bus_post(HTTP_REQ_CHANGE_WF_MODE, (void*)payload);
}

esp_err_t http_service_control_device(int32_t relay, bool state) {
    ESP_LOGI(TAG, "control device request: relay=%d state=%d", relay, state);
    return event_bus_post(HTTP_CONTROL_DEV, NULL);
}

esp_err_t http_service_request_exit(void) {
    ESP_LOGI(TAG, "exit requested via http");
    return event_bus_post(HTTP_EXIT, NULL);
}