/**
 * @file storage_manager.h
 * @brief Storage Manager - Placeholder for future implementation
 * Will be implemented later
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage Manager handle (opaque type)
 * Implementation pending
 */
typedef struct storage_manager storage_manager_t;

/**
 * @brief Initialize Storage Manager
 * 
 * @return Pointer to storage_manager_t instance, or NULL on failure
 */
storage_manager_t* storage_manager_init(void);

/**
 * @brief Read data from storage
 * 
 * @param manager Pointer to storage_manager_t instance
 * @param key Storage key
 * @param data Buffer to store read data
 * @param data_len Buffer length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_read(storage_manager_t* manager, const char* key, 
                               void* data, size_t* data_len);

/**
 * @brief Write data to storage
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

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_MANAGER_H */