#include "http_json_helper.h"
#include "event_bus.h"

const char* g_http_tag = "HTTP_EVENT";
static httpd_handle_t http_server_handle = NULL;

static void http_wifi_stop_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    stop_http_server();
    ESP_LOGI(g_http_tag, "HTTP server stopped due to WiFi event: %d", (int)event_id);
}

static void http_wifi_start_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    start_http_server();
    ESP_LOGI(g_http_tag, "HTTP server started due to WiFi event: %d", (int)event_id);
}

/**
 * format of API when writing
 * - create root object
 * - add value to root
 * - send root to server as string
 * - return root as string to web (json format) + free root
 * - delete root object
 * 
 * format of API when reading
 * - init root
 * - parse and get value
 * - save value (optional)
 * - return root as string to web (json format) + free root
 */

static esp_err_t set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return ESP_OK;
}

// json main logic send json format as string
esp_err_t json_response_https(httpd_req_t *req, cJSON *root) {
    char *res = cJSON_Print(root);
    httpd_resp_set_type(req, DATA_TYPE_JSON);
    httpd_resp_sendstr(req, res);
    free(res);
    return ESP_OK;
}

esp_err_t json_options_handler(httpd_req_t *req) {
    set_cors_headers(req);
    return httpd_resp_sendstr(req, "");
}

// End point, GET: /api/ping
esp_err_t json_get_ping(httpd_req_t* req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "timestamp", (double)(esp_timer_get_time() / 1000));

    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root); //memleak
    return ret;
}

// End point, GET: /api/data 
esp_err_t json_get_data(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "sensor", 25);
    cJSON_AddStringToObject(root, "state", "ok");
    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);
    return ret;
}

// End point, POST: /api/relay
esp_err_t json_post_relay(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';
    cJSON *root = cJSON_Parse(buf);
    // int relay = cJSON_GetObjectItem(root, "relay")->valueint;
    // bool state = cJSON_IsTrue(cJSON_GetObjectItem(root, "state"));

    cJSON_Delete(root);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ok");
    esp_err_t sta = json_response_https(req, resp);
    cJSON_Delete(resp);
    return sta;
}

// End point, POST: /api/reboot
esp_err_t json_post_reboot(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "rebooting");
    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);
    esp_restart();
    return ret;
}

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

esp_err_t stop_http_server(void) {
    esp_err_t ret = ESP_OK;
    if (http_server_handle) {
        ret = httpd_stop(http_server_handle);
    }

    if (ret == ESP_OK) {
        http_server_handle = NULL;
    }
    else {
        ESP_LOGE(g_http_tag, "ERROR when trying to stop http!");
    }

    return ret;
}

esp_err_t start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16;

    if (httpd_start(&http_server_handle, &config) != ESP_OK) {
        return ESP_FAIL;
    }

    httpd_uri_t uri_s[] = {
        {.uri = ROOT_URI, .method = HTTP_GET, .handler = dashboard_get_handler},
        {.uri = FAVICON_URI, .method = HTTP_GET, .handler = favicon_get_handler},
        {.uri = API_DATA, .method = HTTP_GET, .handler = json_get_data},
        {.uri = API_DATA, .method = HTTP_OPTIONS, .handler = json_options_handler},
        {.uri = API_PING, .method = HTTP_GET, .handler = json_get_ping},
        {.uri = API_PING, .method = HTTP_OPTIONS, .handler = json_options_handler},
        {.uri = API_REBOOT, .method = HTTP_POST, .handler = json_post_reboot},
        {.uri = API_REBOOT, .method = HTTP_OPTIONS, .handler = json_options_handler},
        {.uri = API_RELAY, .method = HTTP_POST, .handler = json_post_relay},
        {.uri = API_RELAY, .method = HTTP_OPTIONS, .handler = json_options_handler},
    };

    for (int i = 0; i < (int)(sizeof(uri_s) / sizeof(uri_s[0])); i++) {
        httpd_register_uri_handler(http_server_handle, &uri_s[i]);
    }

    return ESP_OK;
}