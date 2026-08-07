/**
 * @file wifi_service.c
 * @brief WiFi Service implementation - Handles command queue and event processing
 */

#include "wifi_service.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "wifi_service";

/**
 * @brief WiFi Service internal structure
 */
struct wifi_service {
    wifi_manager_t* manager;       /**< Pointer to WiFi Manager */
    QueueHandle_t cmd_queue;       /**< Command queue */
    TaskHandle_t task_handle;      /**< Service task handle */
    uint8_t running;               /**< Service running flag */
    uint32_t queue_size;           /**< Queue size */
};

static void wifi_service_task(void* pvParameters) {
    wifi_service_t* service = (wifi_service_t*)pvParameters;
    wifi_service_cmd_msg_t cmd_msg;
    
    ESP_LOGI(TAG, "WiFi Service task started");
    
    while (service->running) {
        /* Wait for command from queue */
        if (xQueueReceive(service->cmd_queue, &cmd_msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "Processing command: %d", cmd_msg.cmd);
            
            esp_err_t ret = ESP_OK;
            
            switch (cmd_msg.cmd) {
                case WIFI_CMD_SCAN: {
                    /* TODO: Implement WiFi scan */
                    ESP_LOGI(TAG, "Scan command received");
                    break;
                }
                
                case WIFI_CMD_CONNECT: {
                    /* TODO: Implement WiFi connect */
                    ESP_LOGI(TAG, "Connect command received");
                    break;
                }
                
                case WIFI_CMD_DISCONNECT: {
                    esp_wifi_disconnect();
                    ESP_LOGI(TAG, "Disconnected from WiFi");
                    break;
                }
                
                case WIFI_CMD_START_AP: {
                    wifi_manager_set_mode(service->manager, WIFI_MODE_AP);
                    esp_wifi_start();
                    ESP_LOGI(TAG, "Access Point started");
                    break;
                }
                
                case WIFI_CMD_STOP_AP: {
                    esp_wifi_stop();
                    ESP_LOGI(TAG, "Access Point stopped");
                    break;
                }
                
                case WIFI_CMD_GET_STATUS: {
                    wifi_status_t status;
                    wifi_manager_get_status(service->manager, &status);
                    ESP_LOGI(TAG, "WiFi status: mode=%d, sta_connected=%d", 
                            status.mode, status.sta_connected);
                    break;
                }
                
                case WIFI_CMD_SET_CONFIG: {
                    if (cmd_msg.data && cmd_msg.data_len == sizeof(wifi_config_data_t)) {
                        wifi_config_data_t* config = (wifi_config_data_t*)cmd_msg.data;
                        wifi_manager_set_config(service->manager, config);
                        ESP_LOGI(TAG, "WiFi configuration updated");
                    }
                    break;
                }
                
                case WIFI_CMD_GET_CONFIG: {
                    wifi_config_data_t config;
                    wifi_manager_get_config(service->manager, &config);
                    ESP_LOGI(TAG, "WiFi config retrieved: SSID=%s", config.sta_ssid);
                    break;
                }
                
                case WIFI_CMD_SET_MODE: {
                    if (cmd_msg.data && cmd_msg.data_len == sizeof(wifi_mode_t)) {
                        wifi_mode_t* mode = (wifi_mode_t*)cmd_msg.data;
                        wifi_manager_set_mode(service->manager, *mode);
                        ESP_LOGI(TAG, "WiFi mode set to %d", *mode);
                    }
                    break;
                }
                
                default:
                    ESP_LOGW(TAG, "Unknown command: %d", cmd_msg.cmd);
                    break;
            }
            
            /* Free command data if allocated */
            if (cmd_msg.data) {
                free(cmd_msg.data);
            }
        }
    }
    
    ESP_LOGI(TAG, "WiFi Service task stopped");
    vTaskDelete(NULL);
}

wifi_service_t* wifi_service_init(wifi_manager_t* manager) {
    if (!manager) {
        ESP_LOGE(TAG, "Invalid WiFi manager");
        return NULL;
    }
    
    wifi_service_t* service = calloc(1, sizeof(wifi_service_t));
    if (!service) {
        ESP_LOGE(TAG, "Failed to allocate memory for WiFi service");
        return NULL;
    }
    
    service->manager = manager;
    service->queue_size = 10;
    service->running = 0;
    
    /* Create command queue */
    service->cmd_queue = xQueueCreate(service->queue_size, sizeof(wifi_service_cmd_msg_t));
    if (!service->cmd_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        free(service);
        return NULL;
    }
    
    ESP_LOGI(TAG, "WiFi Service initialized");
    return service;
}

esp_err_t wifi_service_start(wifi_service_t* service) {
    if (!service) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (service->running) {
        ESP_LOGW(TAG, "WiFi Service already running");
        return ESP_OK;
    }
    
    service->running = 1;
    
    /* Create service task */
    BaseType_t ret = xTaskCreate(
        wifi_service_task,
        "wifi_service",
        4096,
        service,
        5,
        &service->task_handle
    );
    
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create WiFi Service task");
        service->running = 0;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WiFi Service started");
    return ESP_OK;
}

void wifi_service_stop(wifi_service_t* service) {
    if (!service || !service->running) {
        return;
    }
    
    service->running = 0;
    
    /* Wait for task to finish */
    if (service->task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(service->task_handle);
        service->task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "WiFi Service stopped");
}

esp_err_t wifi_service_send_command(wifi_service_t* service, const wifi_service_cmd_msg_t* cmd_msg) {
    if (!service || !cmd_msg) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!service->running) {
        ESP_LOGE(TAG, "WiFi Service not running");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Create a copy of the command message */
    wifi_service_cmd_msg_t msg_copy;
    memcpy(&msg_copy, cmd_msg, sizeof(wifi_service_cmd_msg_t));
    
    /* If data is provided, make a copy */
    if (cmd_msg->data && cmd_msg->data_len > 0) {
        msg_copy.data = malloc(cmd_msg->data_len);
        if (!msg_copy.data) {
            ESP_LOGE(TAG, "Failed to allocate memory for command data");
            return ESP_ERR_NO_MEM;
        }
        memcpy(msg_copy.data, cmd_msg->data, cmd_msg->data_len);
    } else {
        msg_copy.data = NULL;
    }
    
    /* Send to queue */
    if (xQueueSend(service->cmd_queue, &msg_copy, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send command to queue");
        if (msg_copy.data) {
            free(msg_copy.data);
        }
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGI(TAG, "Command sent to queue: %d", cmd_msg->cmd);
    return ESP_OK;
}

esp_err_t wifi_service_get_status(wifi_service_t* service, uint8_t* running, uint32_t* queue_count) {
    if (!service || !running || !queue_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *running = service->running;
    *queue_count = uxQueueMessagesWaiting(service->cmd_queue);
    
    return ESP_OK;
}

void wifi_service_deinit(wifi_service_t* service) {
    if (!service) {
        return;
    }
    
    /* Stop service if running */
    wifi_service_stop(service);
    
    /* Delete queue */
    if (service->cmd_queue) {
        vQueueDelete(service->cmd_queue);
    }
    
    free(service);
    ESP_LOGI(TAG, "WiFi Service deinitialized");
}