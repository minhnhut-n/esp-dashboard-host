/**
 * file name: http_manager.h
 * brief: HTTP manager layer - server lifecycle owner.
 * author: minhnhut.n
 */

#ifndef HTTP_MANAGER_H
#define HTTP_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_manager_init(void);
esp_err_t http_manager_deinit(void);
esp_err_t http_manager_start(void);
esp_err_t http_manager_stop(void);
bool http_manager_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_MANAGER_H */
