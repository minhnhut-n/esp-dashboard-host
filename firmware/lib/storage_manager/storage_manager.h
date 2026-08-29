/**
 * @file storage_manager.h
 * @brief Storage Manager - NVS (non-volatile flash) wrapper.
 *
 * Design:
 *  - NVS namespace is created under "WIFI_STORAGE".
 *  - All NVS open/set/commit/erase/get sequences are protected by an
 *    internal mutex (semaphore) so concurrent tasks cannot race on the
 *    flash resource.
 *  - Flash writes can be slow; callers (e.g. wifi_handler) should post
 *    events and perform NVS work inside dedicated FreeRTOS tasks so the
 *    dispatcher task is never blocked.
 *
 * @author minhnhut.n
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVS_STORE_NAME "WIFI_STORAGE"

/** NVS keys for the wifi credential bundle */
#define NVS_WIFI_SSID_KEY "wifi_ssid"
#define NVS_WIFI_PASS_KEY "wifi_pass"
#define NVS_SSID_SIZE 32
#define NVS_PASS_SIZE 64

typedef struct storage_manager storage_manager_t;

storage_manager_t* storage_manager_init(void);
void storage_manager_deinit(storage_manager_t* manager);

esp_err_t storage_manager_read(storage_manager_t* manager, const char* key,
                               void* data, size_t* data_len);
esp_err_t storage_manager_write(storage_manager_t* manager, const char* key,
                                const void* data, size_t data_len);
esp_err_t storage_manager_delete(storage_manager_t* manager, const char* key);

esp_err_t storage_manager_save_wifi_creds(const char* ssid, const char* pass);
esp_err_t storage_manager_load_wifi_creds(char* ssid, size_t ssid_len,
                                          char* pass, size_t pass_len);
esp_err_t storage_manager_clear_wifi_creds(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_MANAGER_H */