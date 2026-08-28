/**
 * file name: http_service.h
 * brief: HTTP service layer - async bridge between HTTP, WiFi and storage.
 * author: minhnhut.n
 */

#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_service_save_wifi_creds(const char* ssid, const char* pass);
esp_err_t http_service_load_wifi_creds(char* ssid, size_t ssid_size,
                                       char* pass, size_t pass_size);
esp_err_t http_service_control_device(int32_t relay, bool state);
esp_err_t http_service_request_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVICE_H */
