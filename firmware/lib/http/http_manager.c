/**
 * file name: http_manager.c
 * brief: HTTP manager layer - httpd server lifecycle owner.
 *
 * Responsibilities:
 *   - own the (singleton) server handle
 *   - start/stop the httpd server and register the REST endpoint table
 *   - keep the server in sync with the WiFi driver lifecycle through
 *     WIFI_EVENT_AP_START/STA_START/AP_STOP/STA_STOP
 *
 * Endpoint implementation lives in http_handler, async business logic in
 * http_service.
 *
 * author: minhnhut.n
 */

#include "http_manager.h"
#include "http_handler.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"

static const char* TAG = "HTTP_MANAGER";

#define HTTP_MANAGER_MAX_URI_HANDLERS 16

/* singleton server instance */
static httpd_handle_t s_http_server = NULL;
static bool           s_initialized = false;

/* ------------------------- endpoint table -------------------------------- */

static const httpd_uri_t uri_s[] = {
    {.uri = HTTP_ROOT_URI,       .method = HTTP_GET,     .handler = dashboard_get_handler},
    {.uri = HTTP_FAVICON_URI,    .method = HTTP_GET,     .handler = favicon_get_handler},
    {.uri = HTTP_API_DATA,       .method = HTTP_GET,     .handler = json_get_data},
    {.uri = HTTP_API_DATA,       .method = HTTP_OPTIONS, .handler = json_options_handler},
    {.uri = HTTP_API_PING,       .method = HTTP_GET,     .handler = json_get_ping},
    {.uri = HTTP_API_PING,       .method = HTTP_OPTIONS, .handler = json_options_handler},
    {.uri = HTTP_API_REBOOT,     .method = HTTP_POST,    .handler = json_post_reboot},
    {.uri = HTTP_API_REBOOT,     .method = HTTP_OPTIONS, .handler = json_options_handler},
    {.uri = HTTP_API_RELAY,      .method = HTTP_POST,    .handler = json_post_relay},
    {.uri = HTTP_API_RELAY,      .method = HTTP_OPTIONS, .handler = json_options_handler},
    {.uri = HTTP_API_WIFI_CRED,  .method = HTTP_GET,     .handler = json_get_wifi_cred},
    {.uri = HTTP_API_WIFI_CRED,  .method = HTTP_POST,    .handler = json_post_wifi_cred},
    {.uri = HTTP_API_WIFI_CRED,  .method = HTTP_OPTIONS, .handler = json_options_handler},
    {.uri = HTTP_API_EXIT,       .method = HTTP_POST,    .handler = json_post_exit},
    {.uri = HTTP_API_EXIT,       .method = HTTP_OPTIONS, .handler = json_options_handler},
};

static esp_err_t http_manager_register_uris(void) {
    for (size_t i = 0; i < sizeof(uri_s) / sizeof(uri_s[0]); i++) {
        esp_err_t err = httpd_register_uri_handler(s_http_server, &uri_s[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "register uri (%s) failed: %s",
                     uri_s[i].uri, esp_err_to_name(err));
            return err;
        }
    }
    return ESP_OK;
}

/* --------------------------- lifecycle ----------------------------------- */

esp_err_t http_manager_start(void) {
    if (s_http_server != NULL) {
        ESP_LOGI(TAG, "HTTP server already running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting HTTP server ...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = HTTP_MANAGER_MAX_URI_HANDLERS;

    if (httpd_start(&s_http_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        s_http_server = NULL;
        return ESP_FAIL;
    }

    esp_err_t err = http_manager_register_uris();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register endpoints failed: %s", esp_err_to_name(err));
        httpd_stop(s_http_server);
        s_http_server = NULL;
        return err;
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}

esp_err_t http_manager_stop(void) {
    esp_err_t ret = ESP_OK;
    if (s_http_server != NULL) {
        ret = httpd_stop(s_http_server);
    }

    if (ret == ESP_OK) {
        s_http_server = NULL;
    } else {
        ESP_LOGE(TAG, "ERROR when trying to stop http!");
    }

    return ret;
}

bool http_manager_is_running(void) {
    return s_http_server != NULL;
}

/* --------------------- wifi auto start/stop handlers ---------------------- */

/* esp_event signature: (handler arg, event base, id, event data). */
static void http_wifi_start_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
    (void)arg;
    (void)event_data;
    http_manager_start();
    ESP_LOGI(TAG, "HTTP server started due to WiFi event: %d", (int)event_id);
}

static void http_wifi_stop_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
    (void)arg;
    (void)event_data;
    http_manager_stop();
    ESP_LOGI(TAG, "HTTP server stopped due to WiFi event: %d", (int)event_id);
}

esp_err_t http_manager_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }

    /* keep the dashboard reachable in both AP and STA modes */
    ESP_LOGI(TAG, "HTTP manager init ...");

    esp_err_t err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START,
                                               http_wifi_start_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register AP_START handler failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START,
                                     http_wifi_start_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register STA_START handler failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP,
                                     http_wifi_stop_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register AP_STOP handler failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP,
                                     http_wifi_stop_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register STA_STOP handler failed: %s", esp_err_to_name(err));
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "HTTP manager initialized");
    return ESP_OK;
}

esp_err_t http_manager_deinit(void) {
    http_manager_stop();

    if (s_initialized) {
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_START, http_wifi_start_handler);
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_START, http_wifi_start_handler);
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STOP, http_wifi_stop_handler);
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_STOP, http_wifi_stop_handler);
        s_initialized = false;
    }

    return ESP_OK;
}