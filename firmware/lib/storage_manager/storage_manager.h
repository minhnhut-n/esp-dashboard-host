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

/**
 * @brief Storage Manager handle (opaque type)
 */
typedef struct storage_manager storage_manager_t;

/**
 * @brief Initialize Storage Manager (idempotent singleton).
 *
 * Initializes the NVS flash partition if needed and creates the internal
 * mutex used to lock the flash resource.
 *
 * @return Pointer to storage_manager_t instance, or NULL on failure
 */
storage_manager_t* storage_manager_init(void);

/**
 * @brief Read a blob from storage
 *
 * The storage resource is locked with the internal semaphore for the
 * whole nvs_open -> nvs_get_blob -> nvs_close sequence.
 *
 * @param manager Pointer to storage_manager_t instance
 * @param key Storage key
 * @param data Buffer to store read data
 * @param data_len In: buffer length. Out: actual bytes read.
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_read(storage_manager_t* manager, const char* key,
                               void* data, size_t* data_len);

/**
 * @brief Write a blob to storage
 *
 * The storage resource is locked with the internal semaphore for the
 * whole nvs_open -> nvs_set_blob -> nvs_commit -> nvs_close sequence.
 *
 * @param manager Pointer to storage_manager_t instance
 * @param key Storage key
 * @param data Data to write
 * @param data_len Data length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_write(storage_manager_t* manager, const char* key,
                                const void* data, size_t data_len);

/**
 * @brief Delete data from storage
 *
 * @param manager Pointer to storage_manager_t instance
 * @param key Storage key to delete
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_delete(storage_manager_t* manager, const char* key);

/**
 * @brief Deinitialize Storage Manager and free resources
 *
 * @param manager Pointer to storage_manager_t instance
 */
void storage_manager_deinit(storage_manager_t* manager);

/* ------------------- WiFi credential helpers --------------------------- */

/**
 * @brief Save wifi ssid/pass to flash (NVS under WIFI_STORAGE namespace).
 *
 * Convenience helper wrapping storage_manager_write() with the wifi keys.
 *
 * @param ssid Null-terminated SSID
 * @param pass Null-terminated password
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_save_wifi_creds(const char* ssid, const char* pass);

/**
 * @brief Load wifi ssid/pass from flash.
 *
 * @param ssid Buffer to receive null-terminated SSID
 * @param ssid_len Size of ssid buffer
 * @param pass Buffer to receive null-terminated password
 * @param pass_len Size of pass buffer
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if not stored,
 *         error code otherwise
 */
esp_err_t storage_manager_load_wifi_creds(char* ssid, size_t ssid_len,
                                          char* pass, size_t pass_len);

/**
 * @brief Clear saved wifi credentials from flash.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_clear_wifi_creds(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_MANAGER_H */