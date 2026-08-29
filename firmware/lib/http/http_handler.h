/**
 * file name: http_handler.h
 * brief: HTTP handler layer - endpoint handlers, JSON helpers, dashboard page.
 * author: minhnhut.n
 */

#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------- HTTP content types ------------------------- */
#define DATA_TYPE_JSON   "application/json"
#define GUI_TYPE_HTML    "text/html"
#define IMAGE_TYPE_ICON  "image/x-icon"

/* ------------------- REST endpoint URIs ------------------------- */
#define HTTP_FAVICON_URI    "/favicon.ico"
#define HTTP_ROOT_URI       "/"
#define HTTP_API_DATA       "/api/data"
#define HTTP_API_PING       "/api/ping"
#define HTTP_API_REBOOT     "/api/reboot"
#define HTTP_API_RELAY      "/api/relay"
#define HTTP_API_WIFI_CRED  "/api/wifi_cred"
#define HTTP_API_WIFI_MODE  "/api/wifi_mode"
#define HTTP_API_EXIT       "/api/exit"

#define NVS_SSID_SIZE 32
#define NVS_PASS_SIZE 64

/* ------------------- page handlers ------------------------------ */
esp_err_t dashboard_get_handler(httpd_req_t* req);
esp_err_t favicon_get_handler(httpd_req_t* req);

/* ------------------- json helpers ------------------------- */
esp_err_t json_response_https(httpd_req_t* req, cJSON* root);
esp_err_t json_options_handler(httpd_req_t* req);

/* ------------------- REST endpoint handlers --------------- */
esp_err_t json_get_data(httpd_req_t* req);
esp_err_t json_get_ping(httpd_req_t* req);
esp_err_t json_post_reboot(httpd_req_t* req);
esp_err_t json_post_relay(httpd_req_t* req);
esp_err_t json_get_wifi_cred(httpd_req_t* req);
esp_err_t json_post_wifi_cred(httpd_req_t* req);
esp_err_t json_post_wifi_mode(httpd_req_t* req);
esp_err_t json_post_exit(httpd_req_t* req);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_HANDLER_H */