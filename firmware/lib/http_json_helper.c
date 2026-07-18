#include "http_json_helper.h"

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

// json main logic send json format as string
esp_err_t json_response_https(httpd_req_t *req, cJSON *root) {
    char *res = cJSON_Print(root);
    httpd_resp_set_type(req, DATA_TYPE_JSON);
    httpd_resp_sendstr(req, res);
    free(res);
    return ESP_OK;
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
    return json_response_https(req, root);
}

// End point, POST: /api/relay
esp_err_t json_post_relay(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';
    cJSON *root = cJSON_Parse(buf);
    int relay = cJSON_GetObjectItem(root, "relay")->valueint;
    bool state = cJSON_IsTrue(cJSON_GetObjectItem(root, "state"));

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

esp_err_t start_http_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 10;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        //registry URI handler, in runtime (possible)
        httpd_uri_t uri_s[] = {
            {.uri = API_DATA, .method = HTTP_GET, .handler = json_get_data},
            {.uri = API_PING, .method = HTTP_GET, .handler = json_get_ping},
            {.uri = API_REBOOT, .method = HTTP_POST, .handler = json_post_reboot},
            {.uri = API_RELAY, .method = HTTP_POST, .handler = json_post_relay},
        };
        //registry one by one
        for (int i=0; i< sizeof(uri_s)/sizeof(uri_s[0]); i++) {
            httpd_register_uri_handler(server, &uri_s[i]);
        }
    };
    return ESP_OK;
}