#ifndef _DASHBOARD_PAGE_H_
#define _DASHBOARD_PAGE_H_

#include "esp_err.h"          // Defines esp_err_t
#include "esp_http_server.h"  // Defines httpd_req_t

esp_err_t dashboard_get_handler(httpd_req_t *req);
esp_err_t favicon_get_handler(httpd_req_t *req);

#endif
