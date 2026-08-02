#include "http_service.h"
#include "event_bus.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>

const char* g_http_tag = "HTTP_EVENT";
static httpd_handle_t http_server_handle = NULL;

// save configuration

void save_wifi_creds(const char* ssid, const char* pass) {
    nvs_handle_t nvs_tag;
    esp_err_t err;

    err = nvs_open(NVS_STORE_NAME, NVS_READWRITE, &nvs_tag);
    if (err != ESP_OK) {
        return;
    }

    nvs_set_str(nvs_tag, WIFI_KEY, ssid);
    nvs_set_str(nvs_tag, WIFI_PASS, pass);
    err = nvs_commit(nvs_tag);
    if (err != ESP_OK) {
        ESP_LOGE(g_http_tag, "ESP FAIL ON STORING WIFI CREDS");
    }
    else {
        ESP_LOGI(g_http_tag, "Save wifi credential success!, ssid: %s, pass: %s", ssid, pass);
    }
    nvs_close(nvs_tag);
}

void load_wifi_creds(char* ssid_out, size_t ssid_size, char* pass_out, size_t pass_size) {
    nvs_handle_t nvs_tag;
    esp_err_t err;

    if (ssid_out != NULL && ssid_size > 0) {
        ssid_out[0] = '\0';
    }
    if (pass_out != NULL && pass_size > 0) {
        pass_out[0] = '\0';
    }

    err = nvs_open(NVS_STORE_NAME, NVS_READONLY, &nvs_tag);

    if (err != ESP_OK) {
        ESP_LOGE(g_http_tag, "NO Credential have been saved");
    }
    else {
        size_t ssid_len = ssid_size;
        size_t pass_len = pass_size;

        esp_err_t ssid_err = nvs_get_str(nvs_tag, WIFI_KEY, ssid_out, &ssid_len);
        esp_err_t pass_err = nvs_get_str(nvs_tag, WIFI_PASS, pass_out, &pass_len);

        if (ssid_err == ESP_OK && pass_err == ESP_OK) {
            ESP_LOGI(g_http_tag, "Get wifi credential! ssid: %s, pass: %s", ssid_out, pass_out);
        }
        else {
            if (ssid_err != ESP_OK && ssid_err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(g_http_tag, "Failed to read SSID from NVS: %s", esp_err_to_name(ssid_err));
            }
            if (pass_err != ESP_OK && pass_err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(g_http_tag, "Failed to read password from NVS: %s", esp_err_to_name(pass_err));
            }
        }
    }
    nvs_close(nvs_tag);
}



void http_wifi_stop_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    stop_http_server();
    ESP_LOGI(g_http_tag, "HTTP server stopped due to WiFi event: %d", (int)event_id);
}

void http_wifi_start_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
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
    ESP_LOGI(g_http_tag, "GET %s", req->uri);
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
    if (root == NULL) {
        ESP_LOGE(g_http_tag, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    // int relay = cJSON_GetObjectItem(root, "relay")->valueint;
    // bool state = cJSON_IsTrue(cJSON_GetObjectItem(root, "state"));

    cJSON_Delete(root);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ok");
    esp_err_t sta = json_response_https(req, resp);
    cJSON_Delete(resp);
    return sta;
}

// End point, POST: /api/wifi_cred
esp_err_t json_post_wifi_cred(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        ESP_LOGE(g_http_tag, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Process the WiFi credentials (example logic - replace with actual implementation)
    const char *ssid = cJSON_GetObjectItem(root, "ssid") ? cJSON_GetObjectItem(root, "ssid")->valuestring : NULL;
    const char *password = cJSON_GetObjectItem(root, "password") ? cJSON_GetObjectItem(root, "password")->valuestring : NULL;

    if (!ssid || !password) {
        ESP_LOGE(g_http_tag, "Missing WiFi credentials");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing WiFi credentials");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    char ssid_in[MAX_SSID_LEN] = {0};
    char pass_in[MAX_PASS_LEN] = {0};
    snprintf(ssid_in, sizeof(ssid_in), "%s", ssid);
    snprintf(pass_in, sizeof(pass_in), "%s", password);
    save_wifi_creds(ssid_in, pass_in);

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

// End point, POST: /api/exit
esp_err_t json_post_exit(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "exiting");
    esp_err_t ret = json_response_https(req, root);
    //post http exit event to event bus
    event_bus_post_http_exit();
    cJSON_Delete(root);
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
    if (http_server_handle != NULL) {
        ESP_LOGI(g_http_tag, "HTTP server already running");
        return ESP_OK;
    }

    ESP_LOGI(g_http_tag, "Starting HTTP server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16;

    if (httpd_start(&http_server_handle, &config) != ESP_OK) {
        return ESP_FAIL;
    }

    ESP_LOGI(g_http_tag, "HTTP server started");

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
        {.uri = API_WIFI_CRED, .method = HTTP_POST, .handler = json_post_wifi_cred},
        {.uri = API_WIFI_CRED, .method = HTTP_OPTIONS, .handler = json_options_handler},
        {.uri = API_EXIT, .method = HTTP_POST, .handler = json_post_exit},
        {.uri = API_EXIT, .method = HTTP_OPTIONS, .handler = json_options_handler},
    };

    for (int i = 0; i < (int)(sizeof(uri_s) / sizeof(uri_s[0])); i++) {
        httpd_register_uri_handler(http_server_handle, &uri_s[i]);
    }

    return ESP_OK;
}