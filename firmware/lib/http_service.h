#ifndef _HTTP_SERVICE_H_
#define _HTTP_SERVICE_H_

#include <cJSON.h>
#include <esp_http_server.h>
#include <esp_timer.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "dashboard_page.h"

#define WIFI_KEY "wifi_ssid"
#define WIFI_PASS "wifi_pass"
#define NVS_STORE_NAME "fstorage"

#define DATA_TYPE_JSON "application/json"
#define DATA_TYPE_TEXT "text/plain"
#define DATA_TYPE_BINA "application/octet-stream"

#define GUI_TYPE_HTML "text/html"
#define GUI_TYPE_CSS "text/css"
#define GUI_TYPE_JSP "text/javascript"

#define IMAGE_TYPE_PNG "image/png"
#define IMAGE_TYPE_JPEG "image/jpeg"
#define IMAGE_TYPE_ICON "image/x-icon"

#define FORM_TYPE_URLENCODED "application/x-www-form-urlencoded"
#define FORM_TYPE_MULTIPART "multipart/form-data"

//END POINT HANDLER, API PROVIDE (EDITABLE)
#define API_DATA "/api/data"
#define API_RELAY "/api/relay"
#define API_REBOOT "/api/reboot"
#define API_PING "/api/ping"
#define API_WIFI_CRED "/api/wifi_cred"
#define API_EXIT "/api/exit"
#define ROOT_URI "/"
#define FAVICON_URI "/favicon.ico"

extern const char* g_http_tag;

void save_wifi_creds(const char* ssid, const char* pass);
void load_wifi_creds(char* ssid_out, size_t ssid_size, char* pass_out, size_t pass_size);

//API HANDLE
esp_err_t json_response_https(httpd_req_t *req, cJSON *root);

void http_wifi_stop_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void http_wifi_start_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

esp_err_t json_get_data(httpd_req_t *req);
esp_err_t json_get_ping(httpd_req_t* req);
esp_err_t json_post_relay(httpd_req_t *req);
esp_err_t json_post_reboot(httpd_req_t *req);
esp_err_t json_post_wifi_cred(httpd_req_t *req);
esp_err_t start_http_server(void);
esp_err_t stop_http_server(void);
#endif //_HTTP_SERVICE_H_
